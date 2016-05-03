/*
 * Madrigal
 *
 * Copyright (c) 2016 Craig Drummond <craig.p.drummond@gmail.com>
 *
 * ----
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "upnp/mediaserver.h"
#include "core/networkaccessmanager.h"
#include "core/debug.h"
#include "core/roles.h"
#include <QXmlStreamReader>
#include <QRegularExpression>
#include <QMimeData>
#include <QByteArrayList>

const char * Upnp::MediaServer::constContentDirService="urn:schemas-upnp-org:service:ContentDirectory:1";
static const int constBrowseChunkSize=500;
static const int constSearchChunkSize=100;
static const int constMaxSearchResults=2000;
static const int constSearchTimeout=10000;
static const int constCommandTimeout=15000;
static const char * constIdProperty="id";
static const QByteArray constRootId("0");

static const QByteArray & itemId(Upnp::Device::Item * item) {
    return item ? item->isCollection() ? static_cast<Upnp::MediaServer::Collection *>(item)->id
                                       : static_cast<Upnp::MediaServer::Track *>(item)->id
                : constRootId;
}

Upnp::MediaServer::Track::Track(const QByteArray &i, const QMap<QString, QString> &values, Item *p, int r)
    : Upnp::Device::MusicTrack(values, p, r)
    , id(i)
{
    // Attemt to determine album-artist
    if (albumArtist.isEmpty() && parent) {
        if (Collection::Type_Artist==parent->type()) {
            albumArtist=parent->name;
        } else if (Collection::Type_Album==parent->type()) {
            if (parent->parent && Collection::Type_Artist==parent->parent->type()) {
                albumArtist=parent->parent->name;
            } else if (!static_cast<Album *>(parent)->artist.isEmpty()) {
                albumArtist=static_cast<Album *>(parent)->artist;
            }
        }
    }

    // Only want to show album-art if parent is not an album
    if (parent && Collection::Type_Album==parent->type()) {
        artUrl=QString();
    }

    QMap<QString, QString>::ConstIterator it=values.constBegin();
    QMap<QString, QString>::ConstIterator end=values.constEnd();
    QString resKey=QLatin1String("res.");
    for (; it!=end; ++it) {
        if (it.key().startsWith(resKey)) {
            res.insert(it.key().mid(4), it.value());
        }
    }
}

Upnp::MediaServer::MediaServer(const Ssdp::Device &device, DevicesModel *parent)
    : Device(device, parent)
    , searchItem(0)
    , searchStart(0)
    , searchTimer(0)
    , commandTimer(0)
    , updateId(0)
    , numChildrenSkipped(0)
{
    manufacturer=QLatin1String("minimserver.com")==device.manufacturer ? Man_Minim : Man_Other;
}

Upnp::MediaServer::~MediaServer() {
}

void Upnp::MediaServer::clear() {
    cancelCommands();
    Device::clear();
    updateId=0;
    numChildrenSkipped=0;
}

void Upnp::MediaServer::setActive(bool a) {
    if (!a) {
        cancelCommands();
        updateId=0;
        numChildrenSkipped=0;
    }
    Device::setActive(a);
}

QModelIndex Upnp::MediaServer::index(int row, int column, const QModelIndex &parent) const {
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }
    const QList<Item *> *list = children(parent);
    const Item * c = list && row<list->count() ? list->at(row) : 0;
    return c ? createIndex(row, column, (void *)c) : QModelIndex();
}

QModelIndex Upnp::MediaServer::parent(const QModelIndex &child) const
{
    if (!child.isValid()) {
        return QModelIndex();
    }

    Item * i= toItem(child);
    Item * p=i ? i->parent : 0;

    return p ? createIndex(p->row, 0, p) : QModelIndex();
}

int Upnp::MediaServer::rowCount(const QModelIndex &parent) const {
    const QList<Item *> *list = children(parent);
//    DBUG(MediaServers) << (list ? list->count() : 0);
    return list ? list->count() : 0;
}

QVariant Upnp::MediaServer::data(const QModelIndex &index, int role) const {
    const Item *item = toItem(index);

    //    DBUG(Devices) << index.row() << (void *)item << role;
    if (!item) {
        if (Qt::DisplayRole==role) {
            return details.name;
        }
        return QVariant();
    }

    switch (role) {
    case Core::Role_ImageDetails: {
        QVariant var;
        if (Item::Type_MusicTrack!=item->type() || static_cast<const MusicTrack *>(item)->isBroadcast) {
            var.setValue<Core::ImageDetails>(item->cover());
        }
        return var;
    }
    default:
        return Device::data(index, role);
    }
}

bool Upnp::MediaServer::hasChildren(const QModelIndex &index) const {
    Item *item=toItem(index);
    return !item || item->isCollection();
}

bool Upnp::MediaServer::canFetchMore(const QModelIndex &index) const {
    if (index.isValid()) {
        Item *item=toItem(index);
        return item && item->isCollection() && State_Initial==static_cast<Collection *>(item)->state;
    }
    return false;
}

void Upnp::MediaServer::fetchMore(const QModelIndex &index) {
    populate(index);
}

Qt::ItemFlags Upnp::MediaServer::flags(const QModelIndex &index) const {
    if (index.isValid()) {
        return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
    } else {
        return Qt::NoItemFlags;
    }
}

const char constMimeSep('\r');

static QByteArray toHeirarchy(QModelIndex index) {
    QByteArrayList ids;
    while (index.isValid()) {
        if (Upnp::Device::Item::Type_MusicTrack==static_cast<Upnp::Device::Item *>(index.internalPointer())->type()) {
            ids.prepend(static_cast<Upnp::MediaServer::Track *>(index.internalPointer())->id);
        } else {
            ids.prepend(static_cast<Upnp::MediaServer::Collection *>(index.internalPointer())->id);
        }
        index=index.parent();
    }
    return ids.join(constMimeSep);
}

static QList<QByteArray> toHierarchyList(const QByteArray &list) {
    return list.split(constMimeSep);
}

struct Index : public QModelIndex {
    Index(const QModelIndex &i)
        : QModelIndex(i)
    {
        QModelIndex idx=i;
        while (idx.isValid()) {
            rows.prepend(idx.row());
            idx=idx.parent();
        }
        count=rows.count();
    }

    bool operator<(const Index &rhs) const {
        int toCompare=qMax(count, rhs.count);
        for (int i=0; i<toCompare; ++i) {
            qint32 left=i<count ? rows.at(i) : -1;
            qint32 right=i<rhs.count ? rhs.rows.at(i) : -1;
            if (left<right) {
                return true;
            } else if (left>right) {
                return false;
            }
        }
        return false;
    }

    QList<qint32> rows;
    int count;
};

static QModelIndexList sortIndexes(const QModelIndexList &list) {
    if (list.isEmpty()) {
        return list;
    }

    // QModelIndex::operator< sorts on row first - but this messes things up if rows
    // have different parents. Therefore, we use the sort above - so that the hierarchy is preserved.
    // First, create the list of 'Index' items to be sorted...
    QList<Index> toSort;
    foreach (const QModelIndex &i, list) {
        if (0==i.column()) {
            toSort.append(Index(i));
        }
    }
    // Call qSort on these - this will use operator<
    qSort(toSort);

    // Now convert the QList<Index> into a QModelIndexList
    QModelIndexList sorted;
    foreach (const Index &i, toSort) {
        sorted.append(i);
    }
    return sorted;
}

QStringList Upnp::MediaServer::mimeTypes() const {
    return QStringList() << constObjectIdListMimeType;
}

QMimeData * Upnp::MediaServer::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = new QMimeData();
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    stream << uuid();
    foreach(QModelIndex index, indexes) {
        stream << toHeirarchy(index);
    }

    DBUG(MediaServers) << data.length();
    mimeData->setData(constObjectIdListMimeType, data);
    return mimeData;
}

QModelIndex Upnp::MediaServer::searchIndex() const {
    return searchItem ? createIndex(searchItem->row, 0, searchItem) : QModelIndex();
}

void Upnp::MediaServer::play(const QModelIndexList &indexes, qint32 pos, PlayCommand::Type type) {
    DBUG(MediaServers) << indexes.count() << pos << type;
    if (!command.toPopulate.isEmpty()) {
        command.reset();
    }

    emit info(tr("Locating tracks..."), Notif_PlayCommand);

    if (!commandTimer) {
        commandTimer=new QTimer(this);
        commandTimer->setSingleShot(true);
        connect(commandTimer, SIGNAL(timeout()), this, SLOT(commandTimeout()));
    }
    commandTimer->start(constCommandTimeout);
    command.pos=pos;
    command.type=type;
    foreach (const QModelIndex &idx, indexes) {
        DBUG(MediaServers) << idx.data().toString();
        populateCommand(idx);
    }

    checkCommand();
}

Upnp::Device::Item * findItem(QList<Upnp::Device::Item *> &list, QList<QByteArray> ids) {
    if (ids.isEmpty()) {
        return 0;
    }

    QByteArray toFind=ids.takeFirst();
    foreach (Upnp::Device::Item *item, list) {
//        if (Upnp::Device::Item::Type_MusicTrack==item->type() && static_cast<Upnp::MediaServer::Track *>(item)->id==toFind) {
//            return item;
//        }
        if (item->isCollection() && static_cast<Upnp::MediaServer::Collection *>(item)->id==toFind) {
            return ids.isEmpty() ? item : findItem(static_cast<Upnp::MediaServer::Collection *>(item)->children, ids);
        }
    }
    return 0;
}

void Upnp::MediaServer::play(const QList<QByteArray> &ids, qint32 row) {
    DBUG(MediaServers) << ids << row;
    QList<QByteArray> parentHeirarchy;
    QList<Item *> *lastParentList=&items;
    QModelIndexList indexes;

    foreach (const QByteArray &i, ids) {
        QList<QByteArray> h=toHierarchyList(i);
        if (!h.isEmpty()) {
            QByteArray id=h.takeLast();
            QList<Item *> *list=&items;
            if (!h.isEmpty()) {
                if (h==parentHeirarchy) {
                    list=lastParentList;
                } else {
                    Item *p=::findItem(items, h);
                    if (!p || !p->isCollection()) {
                        DBUG(MediaServers) << "ERROR: Failed to find parent" << h;
                        return;
                    }
                    list=lastParentList=&(static_cast<Upnp::MediaServer::Collection *>(p)->children);
                }
            }
            Item *found=0;
            foreach (Item *item, *list) {
                if (item->isCollection() && static_cast<Upnp::MediaServer::Collection *>(item)->id==id) {
                    DBUG(MediaServers) << "C" << item->name << static_cast<Upnp::MediaServer::Collection *>(item)->id;
                    found=item;
                    break;
                } else if (!item->isCollection() && static_cast<Upnp::MediaServer::Track *>(item)->id==id) {
                    DBUG(MediaServers) << "T" << item->name << static_cast<Upnp::MediaServer::Collection *>(item)->id;
                    found=item;
                    break;
                }
            }
            if (found) {
                indexes.append(createIndex(found->row, 0, found));
            } else {
                DBUG(MediaServers) << "ERROR: Failed to find" << id;
            }
        }
    }

    if (!indexes.isEmpty()) {
        play(indexes, row, PlayCommand::Insert);
    }
}

void Upnp::MediaServer::search(const QString &text) {
    QString trimmed=text.trimmed();
    if (currentSearch==trimmed) {
        return;
    }
    currentSearch=trimmed;
    removeSearchItem();
    if (!currentSearch.isEmpty()) {
        beginInsertRows(QModelIndex(), items.count(), items.count());
        searchItem=new Search(QObject::tr("Search: %1").arg(currentSearch), 0, items.count());
        items.append(searchItem);
        endInsertRows();
        emit searching(true);
        if (!searchTimer) {
            searchTimer=new QTimer(this);
            searchTimer->setSingleShot(true);
            connect(searchTimer, SIGNAL(timeout()), this, SLOT(searchTimeout()));
        }
        searchTimer->start(constSearchTimeout);
        search(0);
    }
}

void Upnp::MediaServer::searchTimeout() {
    emit searching(false);
    Device::cancelCommands("Search");
}

void Upnp::MediaServer::commandTimeout() {
    command.reset();
    emit info(tr("Timeout!"), Notif_PlayCommand, constNotifTimeout);
}

void Upnp::MediaServer::search(quint32 start) {
    QByteArray searchTerm=QByteArray("&quot;")+currentSearch.toHtmlEscaped().toLatin1()+QByteArray("&quot;");
    QByteArray searchString;
    foreach (const QByteArray &cap, searchCap) {
        if (!searchString.isEmpty()) {
            searchString+=" or ";
        }
        searchString+=cap+" contains "+searchTerm;
    }
    searchString="(upnp:class derived from &quot;object.item.audioItem&quot; and ("+searchString+"))";
    searchStart=start;
    sendCommand("<ContainerID>0</ContainerID><SearchCriteria>"+searchString+"</SearchCriteria><Filter>*</Filter>"
                "<SortCriteria></SortCriteria><StartingIndex>"+QByteArray::number(searchStart)+"</StartingIndex><RequestedCount>"+
                QByteArray::number(constSearchChunkSize)+"</RequestedCount>",
                "Search", constContentDirService, true);
}

void Upnp::MediaServer::populate() {
    if (items.isEmpty()) {
        DBUG(MediaServers);
        sendCommand(QByteArray(), "GetSearchCapabilities", constContentDirService);
        populate(QModelIndex());
    }
}

void Upnp::MediaServer::populate(const QModelIndex &index, int start) {
    DBUG(Devices) << index.row();
    Item *item=toItem(index);
    QByteArray id=itemId(item);

    if (item && item->isCollection() && static_cast<Collection *>(item)->state!=State_Populating) {
        static_cast<Collection *>(item)->state=State_Populating;
        emit dataChanged(index, index);
    }

    // TODO: Specify sort
    sendCommand("<ObjectID>"+id+"</ObjectID><BrowseFlag>BrowseDirectChildren</BrowseFlag><Filter>*</Filter>"
                "<SortCriteria></SortCriteria><StartingIndex>"+QByteArray::number(start)+
                "</StartingIndex><RequestedCount>"+QByteArray::number(constBrowseChunkSize)+"</RequestedCount>",
                "Browse", constContentDirService)->setProperty(constIdProperty, id);
}

void Upnp::MediaServer::commandResponse(QXmlStreamReader &reader, const QByteArray &type, Core::NetworkJob *job) {
    if ("GetSearchCapabilities"==type) {
        parseSearchCapabilities(reader);
        return;
    }
    int total=0;
    int returned=0;
    QModelIndex browseParent;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (QLatin1String("Result")==reader.name()) {
                QXmlStreamReader result(reader.readElementText());
                if ("Browse"==type) {
                    browseParent=parseBrowse(result);
                    //                if (constRootId==id) {
                    //                    setState(State_Populated); ??????
                    //                }
                } else if ("Search"==type) {
                    parseSearch(result);
                }
            } else if (QLatin1String("NumberReturned")==reader.name()) {
                returned=reader.readElementText().toUInt();
            } else if (QLatin1String("TotalMatches")==reader.name()) {
                total=reader.readElementText().toUInt();
            }
        }
    }
    if ("Browse"==type) {
        if (!browseParent.isValid() && constRootId!=job->property(constIdProperty).toByteArray()) {
            // If we browse to a collection that has no children, the parseBrowse() will return QModelIndex()
            // So, if this is the case (and id is *not* for the root colection) then use the id to locate
            // the actual parent.
            browseParent=findItem(job->property(constIdProperty).toByteArray(), QModelIndex());
        }
        Collection *col=browseParent.isValid() && static_cast<Item *>(browseParent.internalPointer())->isCollection()
                        ? static_cast<Collection *>(browseParent.internalPointer()) : 0;
        QList<Item *> &list=col ? col->children : items;
        quint32 skipped = col ? col->numChildrenSkipped : numChildrenSkipped;
        if ((list.count()+skipped)==total) {
            checkCommand(browseParent);
        } else {
            populate(browseParent, list.count()+skipped);
        }
    } else if ("Search"==type) {
        if (0==total && 0==returned) {
            emit searching(false);
            emit info(tr("No tracks found!"), Notif_SearchResult, constNotifTimeout);
            removeSearchItem();
            if (searchTimer) {
                searchTimer->stop();
            }
        } else if (returned+searchStart<total && returned+searchStart<constMaxSearchResults) {
            search(searchStart+returned);
        } else {
            if (searchItem) {
                foreach (Item *c, searchItem->children) {
                    static_cast<Collection *>(c)->state=State_Populated;
                }
            }

            if (total>constMaxSearchResults) {
                emit info(tr("Too many matches. Only displaying first %1 tracks.").arg(constMaxSearchResults), Notif_SearchResult, constNotifTimeout);
            }
            emit searching(false);
            if (searchTimer) {
                searchTimer->stop();
            }
        }
    }
}

void Upnp::MediaServer::notification(const QByteArray &sid, const QByteArray &data) {
    Q_UNUSED(sid)
    DBUG(MediaServers) << data;

    QXmlStreamReader reader(data);
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("propertyset")==reader.name()) {
            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.isStartElement() && QLatin1String("property")==reader.name()) {
                    while (!reader.atEnd()) {
                        reader.readNext();
                        if (reader.isStartElement()) {
                            if (QLatin1String("SystemUpdateID")==reader.name()) {
                                quint32 id=reader.readElementText().toUInt();
                                if (id!=updateId) {
                                    updateId=id;
                                    emit systemUpdated();
                                }
                            } else if (QLatin1String("ContainerUpdateIDs")==reader.name()) {
                                // Contents is comma separated list of ids and version values
                                // e.g. ida,vera,idb,verb

                                // First cancel any play, or search, commands...
                                cancelCommands();

                                // Now update modified IDs...
                                QStringList update=reader.readElementText().split(',');
                                if (!update.isEmpty() && 0==update.count()%2) {
                                    for (int i=0; i<update.count(); i+=2) {
                                        updated(update.at(i).toLatin1());
                                    }
                                }
                            }
                        } else if (reader.isEndElement() && QLatin1String("property")==reader.name()) {
                            break;
                        }
                    }
                }
            }
        }
    }
}

static inline QString albumArt(const QString &a) {
    return a.isEmpty() ? Core::Images::self()->constDefaultImage : a;
}

static void fixFolder(Upnp::MediaServer::Folder *folder, Upnp::MediaServer::Manufacturer man) {
    if (Upnp::MediaServer::Man_Minim==man) {
        if (QLatin1String("[folder view]")==folder->name) {
            folder->name=QObject::tr("Folders");
        } else if (QLatin1String("AlbumArtist")==folder->name) {
            folder->name=QObject::tr("Album Artist");
            folder->icn=Core::MonoIcon::user;
        } else if (QLatin1String(">> Complete Album")==folder->name) {
            folder->icn=Core::MonoIcon::ex_cd;
            folder->name=QObject::tr("Show Complete Album");
        }
    }

    if (QLatin1String("Artist")==folder->name || QLatin1String("Artists")==folder->name ||
        QLatin1String("Album Artist")==folder->name || QLatin1String("Album Artists")==folder->name ||
        QLatin1String("All Artists")==folder->name ||
        QLatin1String("Composer")==folder->name || QLatin1String("Conductor")==folder->name) {
        folder->icn=Core::MonoIcon::user;
    } else if (QLatin1String("Album")==folder->name || QLatin1String("Albums")==folder->name) {
        folder->icn=Core::MonoIcon::ex_cd;
    } else if (QLatin1String("Genre")==folder->name) {
        folder->icn=Core::MonoIcon::tags;
    } else if (QLatin1String("Radio")==folder->name) {
        folder->icn=Core::MonoIcon::ex_radio;
    } else if (QLatin1String("Date")==folder->name) {
        folder->icn=Core::MonoIcon::clocko;
    } else if (folder->parent && folder->parent->icon()==Core::MonoIcon::clocko) {
        folder->icn=Core::MonoIcon::clocko;
    } else if (Upnp::MediaServer::Man_Minim==man) {
        if (QLatin1String("1 playlist")==folder->name || QRegExp("^\\d+ playlists$").exactMatch(folder->name)) {
            folder->icn=Core::MonoIcon::listalt;
//            folder->name=QObject::tr("Playlists");
        } else if (QLatin1String("1 artist")==folder->name || QRegExp("^\\d+ artists$").exactMatch(folder->name)) {
            folder->icn=Core::MonoIcon::user;
//            folder->name=QObject::tr("Artists");
        } else if (QLatin1String("1 album")==folder->name || QRegExp("^\\d+ albums$").exactMatch(folder->name)) {
            folder->icn=Core::MonoIcon::ex_cd;
//            folder->name=QObject::tr("Albums");
        } else if (QLatin1String("1 item")==folder->name || QRegExp("^\\d+ items$").exactMatch(folder->name)) {
            folder->icn=Core::MonoIcon::music;
//            folder->name=QObject::tr("Tracks");
        }
    }
}

QModelIndex Upnp::MediaServer::parseBrowse(QXmlStreamReader &reader) {
    QModelIndex parent;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("DIDL-Lite")==reader.name()) {
            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.isStartElement() && (QLatin1String("container")==reader.name() ||
                                                QLatin1String("item")==reader.name())) {
                    QMap<QString, QString> values=objectValues(reader);
                    QByteArray parentId = values["parentID"].toLatin1();
                    QByteArray id = values["id"].toLatin1();
                    // quint32 childcount = attributes.value("childCount").toString().toUInt();

                    if (!parentId.isEmpty() && !id.isEmpty() && values.contains("class")) {
                        if (!parent.isValid() || parentId!=itemId(static_cast<Item *>(parent.internalPointer()))) {
                            parent = findItem(parentId, QModelIndex());
                        }
                        if (parent.isValid() || constRootId==parentId) {
                            Item *item=0;
                            Item *parentItem=parent.isValid() ? static_cast<Item *>(parent.internalPointer()) : 0;
                            QString type=values["class"];
                            QList<Item *> *list=parentItem ? &static_cast<Collection *>(parentItem)->children
                                                           : &items;

                            DBUG(MediaServers) << type << values["title"] << id << parentId;

                            if (QLatin1String("object.container.storageFolder")==type) {
                                Folder *folder=new Folder(values["title"], id, parentItem, list->count());
                                item=folder;
                                fixFolder(folder, manufacturer);
                            } else if (QLatin1String("object.container.genre.musicGenre")==type) {
                                item=new Genre(values["title"], id, parentItem, list->count());
                            } else if (QLatin1String("object.container.person.musicArtist")==type) {
                                item=new Artist(values["title"], id, parentItem, list->count());
                            } else if (QLatin1String("object.container.album.musicAlbum")==type) {
                                item=new Album(values["title"], values[values.contains("artist") ? "artist" : "creator"],
                                        albumArt(values["albumArtURI"]), id, parentItem, list->count());
                            } else if (QLatin1String(constTrackClass)==type ||
                                       QLatin1String(constBroadcastClass)==type) {
                                item=new Track(id, values, parentItem, list->count());
                            } else if (QLatin1String("object.container.playlistContainer")==type) {
                                item=new Playlist(values["title"], id, parentItem, list->count());
                            } else if (type.startsWith(QLatin1String("object.container"))) {
                                // MinimServer...
                                if (QLatin1String(">> Hide Contents")!=values["title"]) {
                                    Folder *folder=new Folder(values["title"], id, parentItem, list->count());
                                    fixFolder(folder, manufacturer);
                                    item=folder;
                                }
                            }

                            if (item) {
                                DBUG(MediaServers) << item->name << item->type();
                                beginInsertRows(parent, list->count(), list->count());
                                list->append(item);
                                endInsertRows();
                            } else if (parentItem && parentItem->isCollection()) {
                                static_cast<Collection *>(parentItem)->numChildrenSkipped++;
                            } else if (constRootId==parentId) {
                                numChildrenSkipped++;
                            }
                        }
                    }
                } else if (reader.isEndElement() && QLatin1String("DIDL-Lite")==reader.name()) {
                    break;
                }
            }
        }
    }
    return parent;
}

void Upnp::MediaServer::parseSearchCapabilities(QXmlStreamReader &reader) {
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("SearchCaps")==reader.name()) {
            QStringList caps=reader.readElementText().split(',');
            foreach (QString cap, caps) {
                if (-1!=cap.indexOf(':') && QLatin1String("dc:date")!=cap && QLatin1String("upnp:actor")!=cap &&
                    QLatin1String("upnp:class")!=cap && QLatin1String("upnp:genre")!=cap) {
                    searchCap.append(cap.replace("\"", "&quot;").toLatin1());
                }
            }
            return;
        }
    }
}

void Upnp::MediaServer::parseSearch(QXmlStreamReader &reader) {
    Album *album=0;
    QModelIndex albumIndex;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement()) {
            if (reader.isStartElement() && QLatin1String("DIDL-Lite")==reader.name()) {
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (reader.isStartElement() && (QLatin1String("container")==reader.name() ||
                                                    QLatin1String("item")==reader.name())) {
                        QMap<QString, QString> values=objectValues(reader);
                        if (QLatin1String(constTrackClass)==values["class"]) {
                            Track *track=new Track(QByteArray(), values);
                            Album *use=0;
                            QModelIndex useIndex;
                            if (!album || album->name!=track->album || album->artist!=track->artistName()) {
                                foreach (Item *child, searchItem->children) {
                                    Album *a=static_cast<Album *>(child);
                                    if (a->name==track->album) {
                                        if (a->artist==track->artistName()) {
                                            use=a;
                                            useIndex=createIndex(use->row, 0, use);
                                        } else if (track->albumArtist.isEmpty() && a->artUrl==track->artUrl) {
                                            use=a;
                                            a->artist=QObject::tr("Various Artists");
                                            useIndex=createIndex(use->row, 0, use);
                                        }
                                    }
                                }
                            } else {
                                use=album;
                                useIndex=albumIndex;
                            }

                            if (!use) {
                                beginInsertRows(createIndex(searchItem->row, 0, searchItem), searchItem->children.count(), searchItem->children.count());
                                use=new Album(track->album, track->artistName(), track->artUrl, QByteArray(), searchItem, searchItem->children.count());
                                use->state=State_Populating;
                                searchItem->children.append(use);
                                endInsertRows();
                                useIndex=createIndex(use->row, 0, use);
                            }
                            track->parent=use;

                            // Ensure correct track order! MiniDLNA sometimes has incorrect order!
                            if (use->children.isEmpty() || track->track>=static_cast<MusicTrack *>(use->children.last())->track) {
                                track->row=use->children.count();
                                beginInsertRows(useIndex, use->children.count(), use->children.count());
                                use->children.append(track);
                                endInsertRows();
                            } else {
                                int numC=use->children.count();
                                for (int r=0; r<numC; ++r) {
                                    if (track->track<static_cast<MusicTrack *>(use->children.at(r))->track) {
                                        track->row=r;
                                        beginInsertRows(useIndex, r, r);
                                        use->children.insert(r, track);
                                        for (int rr=r+1; rr<use->children.count(); ++rr) {
                                            use->children.at(rr)->row=rr;
                                        }
                                        endInsertRows();
                                        break;
                                    }
                                }
                            }
                            track->artUrl=QString();
                            album=use;
                            albumIndex=useIndex;
                        }
                    }
                }
            } else if (reader.isEndElement() && QLatin1String("DIDL-Lite")==reader.name()) {
                break;
            }
        }
    }
}

QModelIndex Upnp::MediaServer::findItem(const QByteArray &id, const QModelIndex &parent) {
    if (constRootId==id) {
        return QModelIndex();
    }

    QList<Item *> *itemList=0;
    if (parent.isValid()) {
        Item *parentItem=static_cast<Item *>(parent.internalPointer());
        if (parentItem->isCollection()) {
            itemList=&static_cast<Collection *>(parentItem)->children;
        }
    } else {
        itemList=&items;
    }

    if (itemList) {
        for (int r=0; r<itemList->count() ; ++r) {
            Item *item=itemList->at(r);
            QModelIndex index=createIndex(r, 0, item);
            if (itemId(item)==id) {
                return index;
            } else if (item->isCollection()) {
                QModelIndex idx=findItem(id, index);
                if (idx.isValid()) {
                    return idx;
                }
            }
        }
    }
    return QModelIndex();
}

const QList<Upnp::Device::Item *> * Upnp::MediaServer::children(const QModelIndex &index) const {
    Item *item=toItem(index);
    return item
            ? item->isCollection()
              ? &static_cast<Collection *>(item)->children
              : 0
            : &items;
}

void Upnp::MediaServer::populateCommand(const QModelIndex &idx) {
    if (Item::Type_MusicTrack==static_cast<Item *>(idx.internalPointer())->type()) {
        DBUG(MediaServers) << "Add track (idx)" << idx.data().toString();
        command.populated.append(idx);
    } else if (canFetchMore(idx)) {
        DBUG(MediaServers) << "Populate" << idx.data().toString();
        command.toPopulate.append(idx);
        fetchMore(idx);
    } else {
        Collection *col=static_cast<Collection *>(idx.internalPointer());
        DBUG(MediaServers) << col->name << col->children.count();
        foreach (Item *item, col->children) {
            QModelIndex child=createIndex(item->row, 0, item);
            if (Item::Type_MusicTrack==item->type()) {
                DBUG(MediaServers) << "Add track (child)" << item->name;
                command.populated.append(child);
            } else {
                populateCommand(child);
            }
        }
    }
}

void Upnp::MediaServer::checkCommand() {
    if (Command::None!=command.type && command.toPopulate.isEmpty()) {
        if (commandTimer) {
            commandTimer->stop();
        }
        QModelIndexList sorted=sortIndexes(command.populated);
        if (!sorted.isEmpty()) {
            Command *cmd=new Command;
            QSet<QString> trackUrls;
            foreach (const QModelIndex &idx, sorted) {
                MusicTrack *trk=new MusicTrack(*static_cast<MusicTrack *>(idx.internalPointer()));
                if (!trackUrls.contains(trk->url)) {
                    trackUrls.insert(trk->url);
                    if (trk->artUrl.isEmpty() && trk->parent && Collection::Type_Album==trk->parent->type()) {
                        trk->artUrl=static_cast<Album *>(trk->parent)->artUrl;
                    }
                    cmd->tracks.append(trk);
                }
            }
            cmd->pos=command.pos;
            cmd->type=command.type;
            DBUG(MediaServers) << cmd->tracks.count();
            emit info(1==cmd->tracks.count() ? tr("Located 1 track") : tr("Located %1 tracks").arg(cmd->tracks.count()), Notif_PlayCommand, constNotifTimeout);
            emit addTracks(cmd);
        } else {
            emit info(tr("No tracks located"), Notif_PlayCommand, constNotifTimeout);
        }
        command.reset();
    }
}

void Upnp::MediaServer::checkCommand(const QModelIndex &idx) {
    if (-1!=command.toPopulate.indexOf(idx)) {
        command.toPopulate.removeAll(idx);
        populateCommand(idx);
    }
    checkCommand();
}

void Upnp::MediaServer::removeSearchItem() {
    if (searchItem) {
        // TODO: Check if command contains search items! If so, then these need to be removed
        beginRemoveRows(QModelIndex(), searchItem->row, searchItem->row);
        items.removeAll(searchItem);
        delete searchItem;
        searchItem=0;
        endRemoveRows();
    }
}

void Upnp::MediaServer::cancelCommands() {
    if (searchTimer && searchTimer->isActive()) {
        searchTimer->stop();
        Device::cancelCommands("Search");
        emit searching(false);
    }
    command.reset();
    if (commandTimer && commandTimer->isActive()) {
        commandTimer->stop();
        emit info(QString(), Notif_PlayCommand);
    }
}

void Upnp::MediaServer::updated(const QByteArray &id) {
    DBUG(MediaServers) << id;

    // See if this id is know to our model

    if (constRootId==id) {
        clear();
        populate(QModelIndex());
        return;
    }

    QModelIndex idx=findItem(id, QModelIndex());
    if (idx.isValid() && static_cast<Item *>(idx.internalPointer())->isCollection()) {
        // Found, remove child items (if it has any) and repopulate
        Collection *col=static_cast<Collection *>(idx.internalPointer());
        if (!col->children.isEmpty()) {
            beginRemoveRows(idx, 0, col->children.count()-1);
            qDeleteAll(col->children);
            col->children.clear();
            endRemoveRows();
            populate(idx);
        }
    }
}
