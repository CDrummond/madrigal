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

#ifndef UPNP_MEDIA_SERVER_H
#define UPNP_MEDIA_SERVER_H

#include "upnp/device.h"
#include "upnp/command.h"
#include "core/actions.h"

class QTimer;
class QXmlStreamReader;

namespace Upnp {

class MediaServer : public Device {
    Q_OBJECT
public:
    enum Manufacturer {
        Man_Other,
        Man_Minim
    };

    struct Collection : public Item {
        enum CollectionType {
            Type_Folder = 101,
            Type_Genre,
            Type_Artist,
            Type_Album,
            Type_Playlist,
            Type_Search
        };

        Collection(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Item(n, p, r), state(State_Initial), id(i), updateId(0), numChildrenSkipped(0) { }
        virtual ~Collection() {
            qDeleteAll(children);
            children.clear();
        }
        virtual bool isCollection() const { return true; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::folder; }

        QList<Item *> children;
        quint16 count;
        State state;
        QByteArray id;
        quint32 updateId;
        quint32 numChildrenSkipped;
    };

    struct Folder : public Collection {
        Folder(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Collection(n, i, p, r), icn(Core::MonoIcon::folder) { }
        virtual ~Folder() { }
        int type() const { return Type_Folder; }
        virtual Core::MonoIcon::Type icon() const { return icn; }
        Core::MonoIcon::Type icn;
    };

    struct Playlist : public Collection {
        Playlist(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Collection(n, i, p, r) { }
        virtual ~Playlist() { }
        int type() const { return Type_Playlist; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::listalt; }
        virtual QVariant actions() const {
            QVariant v;
            v.setValue< QList<int> >(QList<int>() << Core::Actions::Action_Play << Core::Actions::Action_Add);
            return v;
        }
    };

    struct Genre : public Collection {
        Genre(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Collection(n, i, p, r) { }
        virtual ~Genre() { }
        int type() const { return Type_Genre; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::tags; }
    };

    struct Artist : public Collection {
        Artist(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Collection(n, i, p, r) { }
        virtual ~Artist() { }
        int type() const { return Type_Artist; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::user; }
        virtual QVariant actions() const {
            QVariant v;
            v.setValue< QList<int> >(QList<int>() << Core::Actions::Action_Play << Core::Actions::Action_Add);
            return v;
        }
    };

    struct Album : public Collection {
        Album(const QString &n=QString(), const QString &a=QString(), const QString &art=QString(),
              const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : Collection(n, i, p, r), artist(a), artUrl(art) { }
        virtual ~Album() { }
        int type() const { return Type_Album; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::ex_cd; }
        virtual Core::ImageDetails cover() const { return Core::ImageDetails(artUrl, artist, name); }
        virtual QVariant actions() const {
            QVariant v;
            v.setValue< QList<int> >(QList<int>() << Core::Actions::Action_Play << Core::Actions::Action_Add);
            return v;
        }
        QString subText() const { return parent && Type_Artist==parent->type() ? parent->name : artist; }
        QString artist;
        QString artUrl;
    };

    struct Track : public MusicTrack {
        Track(const QByteArray &i, const QMap<QString, QString> &values, Item *p=0, int r=0);
        Track(const QString &n=QString(), const QByteArray &i=QByteArray(), Item *p=0, int r=0)
            : MusicTrack(n, p, r), id(i) { }
        virtual ~Track() { }
        virtual QVariant actions() const {
            QVariant v;
            v.setValue< QList<int> >(isBroadcast
                                        ? QList<int>() << Core::Actions::Action_Play
                                        : QList<int>() << Core::Actions::Action_Play << Core::Actions::Action_Add);
            return v;
        }
        QByteArray id;
    };

    struct Search : public Collection {
        Search(const QString &n=QString(), Item *p=0, int r=0)
            : Collection(n, QByteArray(), p, r) { }
        virtual ~Search() { }
        int type() const { return Type_Search; }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::search; }
    };

    static const char * constContentDirService;

    struct PlayCommand : public Command {
        virtual ~PlayCommand() { tracks.clear(); }
        void reset() {
            type=None;
            pos=0;
            toPopulate.clear();
            populated.clear();
            tracks.clear();
        }
        bool isEmpty() const { return toPopulate.isEmpty() && populated.isEmpty() && tracks.isEmpty(); }
        QModelIndexList populated;
        QModelIndexList toPopulate;
    };

public:
    MediaServer(const Ssdp::Device &device, DevicesModel *parent);
    virtual ~MediaServer();

    virtual void clear();
    virtual void setActive(bool a);
    virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::no_icon==details.icon ? Core::MonoIcon::server : details.icon; }
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &child) const;
    int rowCount(const QModelIndex &parent) const;
    QVariant data(const QModelIndex &index, int role) const;
    bool hasChildren(const QModelIndex &index) const;
    bool canFetchMore(const QModelIndex &index) const;
    void fetchMore(const QModelIndex &index);
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QStringList mimeTypes() const;
    QMimeData * mimeData(const QModelIndexList &indexes) const;
    bool hasCommand() const { return !command.isEmpty(); }
    QModelIndex searchIndex() const;
    void refresh(const QModelIndex &index, bool force=false);
    bool isSearchEnabled() const { return !searchCap.isEmpty(); }

public Q_SLOTS:
    void play(const QModelIndexList &indexes, qint32 pos, PlayCommand::Type type);
    void play(const QList<QByteArray> &ids, qint32 row);
    void search(const QString &text);

private Q_SLOTS:
    void searchTimeout();
    void commandTimeout();

Q_SIGNALS:
    void addTracks(Upnp::Command *cmd);
    void searching(bool);
    void searchEnabled(bool);
    void systemUpdated();

private:
    void search(quint32 start);
    void populate();
    void populate(const QModelIndex &index, int start=0);
    void commandResponse(QXmlStreamReader &reader, const QByteArray &type, Core::NetworkJob *job);
    void notification(const QByteArray &sid, const QByteArray &data);
    QModelIndex parseBrowse(QXmlStreamReader &reader);
    void parseSearchCapabilities(QXmlStreamReader &reader);
    void parseSearch(QXmlStreamReader &reader);
    void parseSystemUpdateId(QXmlStreamReader &reader);
    void checkSystemUpdateId(quint32 val);
    QModelIndex findItem(const QByteArray &id, const QModelIndex &parent);
    const QList<Item *> * children(const QModelIndex &index) const;
    void populateCommand(const QModelIndex &idx);
    void checkCommand();
    void checkCommand(const QModelIndex &idx);
    void removeSearchItem();
    void cancelCommands();

private:
    Manufacturer manufacturer;
    QList<QByteArray> searchCap;
    QString currentSearch;
    Search *searchItem;
    quint32 searchStart;
    QTimer *searchTimer;
    QTimer *commandTimer;
    PlayCommand command;
    quint32 updateId; // UpdateID for root colletion
    quint32 lastColUpdateId; // Last UpdateID received for any collection
    quint32 numChildrenSkipped;
};

}

#endif
