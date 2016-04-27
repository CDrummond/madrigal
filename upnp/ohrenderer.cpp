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

#include "upnp/ohrenderer.h"
#include "upnp/command.h"
#include "core/debug.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QFont>
#ifdef Q_OS_MAC
#include <libkern/OSByteOrder.h>
#define be32toh(x) OSSwapBigToHostInt32(x)
#elif defined Q_OS_WIN
#include <QtEndian>
#define be32toh(x) qFromBigEndian(x)
#else
#include <endian.h>
#endif

const char * Upnp::OhRenderer::constPlaylistService="urn:av-openhome-org:service:Playlist:1";
const char * Upnp::OhRenderer::constRadioService="urn:av-openhome-org:service:Radio:1";
const char * Upnp::OhRenderer::constReceiverService="urn:av-openhome-org:service:Receiver:1";
const char * Upnp::OhRenderer::constSenderService="urn:av-openhome-org:service:Sender:1";
static const char * constProductService="urn:av-openhome-org:service:Product:1";
static const char * constRenderingControlService="urn:schemas-upnp-org:service:RenderingControl:1";
static const int constReadListSize = 20;

static QList<quint32> decodeIds(QXmlStreamReader &reader) {
    QList<quint32> ids;
    QByteArray encoded=reader.readElementText().toLatin1();
    QByteArray idArray=QByteArray::fromBase64(encoded);
    if (idArray.toBase64()==encoded && 0==idArray.length()%4) {
        const char *data=idArray.constData();
        for (int i=0; i<idArray.length(); i+=4) {
            ids.append(be32toh(*(quint32 *)(&data[i])));
        }
    } else {
        // Signal error...
        ids.append(0);
    }
    return ids;
}

static QByteArray valueStr(quint32 val) {
    return "<Value>"+QByteArray::number(val)+"</Value>";
}

static inline bool toBool(const QString &str) {
    return QLatin1String("1")==str;
}

Upnp::OhRenderer::OhRenderer(const Ssdp::Device &device, DevicesModel *parent)
    : Renderer(device, parent)
    , currentCmd(0)
    , addedCount(0)
    , sourceIndex(0)
{
    QList<QByteArray> toRemove;
    Ssdp::Device::Services::ConstIterator it=details.services.constBegin();
    Ssdp::Device::Services::ConstIterator end=details.services.constEnd();
    for(; it!=end; ++it) {
        if (it.key()==constRadioService /* Radio not supported - this is best in server*/ ||
            it.key()==constReceiverService ||
            (!it.key().startsWith("urn:av-openhome-org:service:") && it.key()!=constRenderingControlService)) {
            toRemove.append(it.key());
        }
    }
    foreach (const QByteArray &srv, toRemove) {
        details.services.remove(srv);
    }
}

Upnp::OhRenderer::~OhRenderer() {
    clearCommand();
}

void Upnp::OhRenderer::setActive(bool a) {
    if (!a) {
        clearCommand();
    }
    Renderer::setActive(a);
}

void Upnp::OhRenderer::clear() {
    Device::clear();
}

void Upnp::OhRenderer::populate() {
    if (items.isEmpty()) {
        DBUG(Renderers);
        sendCommand("", "SourceIndex", constProductService);
        sendCommand("", "SourceXml", constProductService);
        // http://wiki.openhome.org/wiki/Av:Developer:PlaylistService
        sendCommand("", "IdArray", constPlaylistService);
        sendCommand("", "Id", constPlaylistService);
        sendCommand("", "Repeat", constPlaylistService);
        sendCommand("", "Shuffle", constPlaylistService);
        sendCommand("", "TransportState", constPlaylistService);
    }
}

void Upnp::OhRenderer::sendPing() {
    // TODO: If Receiver send TransportState
    sendPingCommand("", "Id", constPlaylistService);
}

void Upnp::OhRenderer::commandResponse(QXmlStreamReader &reader, const QByteArray &type, Core::NetworkJob *job) {
    Q_UNUSED(job)
    // TODO: Radio service? Currently disabled in constructor. Need to map URL from job to obtain service type
    if ("IdArray"==type) {
        handleIdArray(reader);
    } else if ("Id"==type) {
        updateCurrentTrackId(getValue(reader));
    } else if ("ReadList"==type) {
        handleReadList(reader);
    } else if ("Repeat"==type) {
        updateRepeat(getValue(reader));
    } else if ("Shuffle"==type) {
        updateShuffle(getValue(reader));
    } else if ("TransportState"==type) {
        updateTransportState(getValue(reader));
    } else if ("Insert"==type) {
        handleInsert(reader);
    } else if ("SourceIndex"==type) {
        handleSourceIndex(getValue(reader).toUInt());
    } else if ("SourceXml"==type) {
        QXmlStreamReader xmlReader(getValue(reader));
        handleSourceXml(xmlReader);
    }
    messageReceived();
}

void Upnp::OhRenderer::failedCommand(Core::NetworkJob *job, const QByteArray &type) {
    DBUG(Renderers) << type;
    Q_UNUSED(job)
    if ("Insert"==type) {
        if (currentCmd) {
            if (Command::ReplaceAndPlay==currentCmd->type) {
                sendCommand("", "Play", constPlaylistService);
            }
            emitAddedTracksNotif();
            clearCommand();
        }
    }
}

void Upnp::OhRenderer::notification(const QByteArray &sid, const QByteArray &data) {
    // TODO: Radio service? Currently disabled in constructor
    Q_UNUSED(sid)
    DBUG(Renderers) << data;
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
                                if (QLatin1String("Seconds")==reader.name()) {
                                    quint32 val=reader.readElementText().toUInt();
                                    if (val!=playState.seconds) {
                                        playState.seconds=val;
                                        emit playbackPos(playState.seconds);
                                    }
                                } else if (QLatin1String("Duration")==reader.name()) {
                                    quint32 val=reader.readElementText().toUInt();
                                    if (val!=playState.duration) {
                                        playState.duration=val;
                                        emit playbackDuration(playState.duration);
                                    }
                                } else if (QLatin1String("IdArray")==reader.name()) {
                                    updateTracks(decodeIds(reader));
                                } else if (QLatin1String("Id")==reader.name()) {
                                    updateCurrentTrackId(reader.readElementText());
                                } else if (QLatin1String("Repeat")==reader.name()) {
                                    updateRepeat(reader.readElementText());
                                } else if (QLatin1String("Shuffle")==reader.name()) {
                                    updateShuffle(reader.readElementText());
                                } else if (QLatin1String("TracksMax")==reader.name()) {
                                    ;
                                } else if (QLatin1String("VolumeSteps")==reader.name()) {
                                    quint32 val=reader.readElementText().toUInt();
                                    if (val!=volState.steps) {
                                        volState.steps=val;
                                        emit volumeState(volState);
                                    }
                                } else if (QLatin1String("VolumeMax")==reader.name()) {
                                    quint32 val=reader.readElementText().toUInt();
                                    if (val!=volState.max) {
                                        volState.max=val;
                                        emit volumeState(volState);
                                    }
                                } else if (QLatin1String("Volume")==reader.name()) {
                                    quint32 val=reader.readElementText().toUInt();
                                    if (val!=volState.current) {
                                        volState.current=val;
                                        emit volumeState(volState);
                                    }
                                } else if (QLatin1String("Mute")==reader.name()) {
                                    bool val=toBool(reader.readElementText());
                                    if (val!=volState.muted) {
                                        volState.muted=val;
                                        emit volumeState(volState);
                                    }
                                } else if (QLatin1String("TransportState")==reader.name()) {
                                    updateTransportState(reader.readElementText());
                                } else if (QLatin1String("Uri")==reader.name()) {
                                    ; // qWarning() << "Uri:" << reader.readElementText();
                                } else if ("SourceIndex"==reader.name()) {
                                    handleSourceIndex(reader.readElementText().toUInt());
                                } else if ("SourceXml"==reader.name()) {
                                    QXmlStreamReader xmlReader(reader.readElementText());
                                    handleSourceXml(xmlReader);
                                }
                           } else if (reader.isEndElement() && QLatin1String("property")==reader.name()) {
                               break;
                           }
                      }
                  }
             }
         }
    }
    messageReceived();
}

void Upnp::OhRenderer::updateTransportState(const QString &val) {
    if (val.isEmpty()) {
        return;
    }
    State state=playState.state;
    if (QLatin1String("Buffering")==val || QLatin1String("Stopped")==val) {
        state=Stopped;
    } else if (QLatin1String("Paused")==val) {
        state=Paused;
    } else if (QLatin1String("Playing")==val) {
        state=Playing;
    }
    if (state!=playState.state) {
        playState.state=state;
        emit playbackState(playState.state);

        if (currentTrackId) {
            qint32 row=getRowById(currentTrackId);
            if (row>=0 && row<items.count()) {
                QModelIndex index=createIndex(row, 0, items.at(row));
                emit dataChanged(index, index);
            }
        }
    }
}

void Upnp::OhRenderer::updateCurrentTrackId(const QString &val) {
    if (val.isEmpty()) {
        return;
    }
    updateCurrentTrackId(val.toUInt());
}

void Upnp::OhRenderer::updateShuffle(const QString &val) {
    if (val.isEmpty()) {
        return;
    }
    bool v=toBool(val);
    if (v!=playState.shuffle) {
        playState.shuffle=v;
        emit shuffle(playState.shuffle);
    }
}

void Upnp::OhRenderer::updateRepeat(const QString &val) {
    if (val.isEmpty()) {
        return;
    }
    if (val.isEmpty()) {
        return;
    }
    bool v=toBool(val);
    if (v!=playState.repeat) {
        playState.repeat=v;
        emit repeat(playState.repeat);
    }
}

void Upnp::OhRenderer::handleIdArray(QXmlStreamReader &reader) {
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("Array")==reader.name()) {
            updateTracks(decodeIds(reader));
            return;
        }
    }
}

void Upnp::OhRenderer::handleInsert(QXmlStreamReader &reader) {
    if (!currentCmd) {
        return;
    }
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("NewId")==reader.name()) {
            quint32 id=reader.readElementText().toUInt();
            if (Command::ReplaceAndPlay==currentCmd->type) {
                // Set command type to something else, as start play at first track
                currentCmd->type=Command::Append;
                sendCommand("<Value>"+QByteArray::number(id)+"</Value>", "SeekId", constPlaylistService);
                sendCommand("", "Play", constPlaylistService);
            }
            addedCount++;
            if (currentCmd->tracks.isEmpty()) {
                emitAddedTracksNotif();
                clearCommand();
            } else {
                const MusicTrack *track=currentCmd->tracks.takeFirst();
                addTrack(track, id);
                delete track;
            }
            return;
        }
    }
}

void Upnp::OhRenderer::handleSourceIndex(quint32 val) {
    DBUG(Renderers) << val << sourceIndex;
    if (val!=sourceIndex) {
        sourceIndex=val;
        // TODO: When handle Radio/SongCast will need to emit!

        if (sourceIndex<sources.count() && sources.at(sourceIndex).type!="Playlist") {
            selectSource("Playlist");
        }
    }
}

void Upnp::OhRenderer::handleSourceXml(QXmlStreamReader &reader) {
    QList<Source> src;
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("Source")==reader.name()) {
            Source s;
            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.isStartElement()) {
                    if (QLatin1String("Name")==reader.name()) {
                        s.name=reader.readElementText();
                    } else if (QLatin1String("Type")==reader.name()) {
                        s.type=reader.readElementText();
                    } else if (QLatin1String("Name")==reader.name()) {
                        s.visible=toBool(reader.readElementText());
                    }
                } else if (reader.isEndElement() && QLatin1String("Source")==reader.name()) {
                    src.append(s);
                    break;
                }
            }
        }
    }

    if (sources!=src || sourceIndex>=sources.count()) {
        DBUG(Renderers) << "sourceXml changed";
        sources=src;
        // TODO: When handle Radio/SongCast will need to emit list!
        selectSource("Playlist");
    }
}

void Upnp::OhRenderer::selectSource(const QString &src) {
    for (quint32 s=0; s<sources.count(); ++s) {
        if (sources.at(s).type==src) {
            if (s!=sourceIndex) {
                DBUG(Renderers) << "set index" << s << src;
                sendCommand(valueStr(s), "SetSourceIndex", constProductService);
            }
            break;
        }
    }
}

QString Upnp::OhRenderer::getValue(QXmlStreamReader &reader) {
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("Value")==reader.name()) {
            return reader.readElementText();
        }
    }
    return 0;
}

void Upnp::OhRenderer::handleReadList(QXmlStreamReader &reader) {
    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("TrackList")==reader.name()) {
            QXmlStreamReader list(reader.readElementText());

            while (!list.atEnd()) {
                list.readNext();
                if (list.isStartElement() && QLatin1String("TrackList")==list.name()) {
                    while (!list.atEnd()) {
                        list.readNext();
                        if (list.isStartElement() && QLatin1String("Entry")==list.name()) {
                            Track meta;
                            QString uri;
                            quint32 id;
                            while (!list.atEnd()) {
                                list.readNext();
                                if (list.isStartElement()) {
                                    if (QLatin1String("Uri")==list.name()) {
                                        uri=list.readElementText();
                                    } else if (QLatin1String("Id")==list.name()) {
                                        id=list.readElementText().toUInt();
                                    } else if (QLatin1String("Metadata")==list.name()) {
                                        meta=Track(0, parseTrackMetadata(list.readElementText()), 0, 0);
                                    }
                                } else if (list.isEndElement() && QLatin1String("Entry")==list.name()) {
                                    break;
                                }
                            }

                            qint32 row=getRowById(id);
                            if (-1!=row) {
                                Track *track=static_cast<Track *>(items.at(row));

                                track->url=uri;
                                track->name=meta.name;
                                track->artist=meta.artist;
                                track->albumArtist=meta.albumArtist;
                                track->creator=meta.creator;
                                track->album=meta.album;
                                track->genre=meta.genre;
                                track->track=meta.track;
                                track->year=meta.year;
                                track->duration=meta.duration;
                                track->artUrl=meta.artUrl;
                                track->isBroadcast=meta.isBroadcast;
                                QModelIndex idx=createIndex(row, 0, track);
                                emit dataChanged(idx, idx);

                                if (id==currentTrackId) {
                                    emit currentTrack(idx);
                                }
                            }
                        }
                    }
                }
            }
        }
    }
    updateStats();
}

qint32 Upnp::OhRenderer::getRowById(quint32 id) const {
    for (qint32 i=0; i<items.count() ; ++i) {
        if (static_cast<Track *>(items.at(i))->id==id) {
            return i;
        }
    }
    return -1;
}

void Upnp::OhRenderer::updateTracks(const QList<quint32> &update) {
    DBUG(Renderers) << update;
    if (1==update.length() && 0==update.first()) {
        return;
    }
    QList<quint32> needDetails;
    QSet<quint32> newIds=update.toSet();

    if (items.isEmpty() || update.isEmpty() || update.count()>8192) {
        beginResetModel();
        qDeleteAll(items);
        items.clear();
        foreach (const quint32 &id, update) {
            items.append(new Track(id, tr("Track %1").arg(items.count()+1)));
        }
        needDetails=update;
        endResetModel();
    } else {
        QSet<quint32> removed=ids-newIds;
        foreach (quint32 id, removed) {
            qint32 row=getRowById(id);
            if (-1!=row) {
                ids.remove(id);
                beginRemoveRows(QModelIndex(), row, row);
                delete items.takeAt(row);
                endRemoveRows();
            }
        }
        for (quint32 i=0; i<update.count(); ++i) {
            quint32 trackId=update.at(i);
            bool newTrack=i>=items.count();
            Track *currentTrackAtPos=newTrack ? 0 : static_cast<Track *>(items.at(i));

            if (newTrack || trackId!=currentTrackAtPos->id) {
                qint32 existingPos=newTrack ? -1 : getRowById(trackId);
                if (-1==existingPos) {
                    beginInsertRows(QModelIndex(), i, i);
                    items.insert(i, new Track(trackId, tr("Track %1").arg(i+1)));
                    if (!ids.contains(trackId)) {
                        needDetails.append(trackId);
                    }
                    endInsertRows();
                } else {
                    beginMoveRows(QModelIndex(), existingPos, existingPos, QModelIndex(), i>existingPos ? i+1 : i);
                    Item *old=items.takeAt(existingPos);
                    items.insert(i, old);
                    endMoveRows();
                }
            }
        }

//        if (items.count()>update.count()) {
//            int toBeRemoved=items.count()-update.count();
//            beginRemoveRows(QModelIndex(), items.count(), update.count()-1);
//            for (int i=0; i<toBeRemoved; ++i) {
//                delete items.takeLast();
//            }
//            endRemoveRows();
//        }
    }

    QList<QByteArray> toSend;
    foreach (const quint32 &id, needDetails) {
        toSend.append(QByteArray::number(id));
        if (constReadListSize==toSend.count()) {
            sendCommand("<IdList>"+toSend.join(' ')+"</IdList>", "ReadList", constPlaylistService);
            toSend.clear();
        }
    }
    if (!toSend.isEmpty()) {
        sendCommand("<IdList>"+toSend.join(' ')+"</IdList>", "ReadList", constPlaylistService);
    }
    ids=newIds;
    updateStats();
}

QMap<QString, QString> Upnp::OhRenderer::parseTrackMetadata(const QString &xml) {
    QXmlStreamReader reader(xml);
    QMap<QString, QString> meta;

    while (!reader.atEnd()) {
        reader.readNext();
        if (reader.isStartElement() && QLatin1String("DIDL-Lite")==reader.name()) {
            while (!reader.atEnd()) {
                reader.readNext();
                if (reader.isStartElement() && QLatin1String("item")==reader.name()) {
                    meta=objectValues(reader);
                    break;
                }
            }
        }
    }

    return meta;
}

void Upnp::OhRenderer::updateCurrentTrackId(quint32 id) {
    if (id!=currentTrackId) {
        qint32 row=getRowById(id);
        if (row>=0 && row<items.count()) {
            currentTrackId=id;
            MusicTrack *track=static_cast<MusicTrack *>(items.at(row));
            QModelIndex index=createIndex(row, 0, track);
            emit currentTrack(index);
            emit dataChanged(index, index);
        }
    }
}

QModelIndex Upnp::OhRenderer::current() {
    if (-1!=currentTrackId) {
        qint32 row=getRowById(currentTrackId);
        if (row>=0 && row<items.count()) {
            return createIndex(row, 0, items.at(row));
        }
    }
    return QModelIndex();
}

void Upnp::OhRenderer::previous() {
    DBUG(Renderers);
    sendCommand("", "Previous", constPlaylistService);
}

void Upnp::OhRenderer::playPause() {
    DBUG(Renderers);
    sendCommand("", Playing==playState.state ? "Pause" : "Play", constPlaylistService);
}

void Upnp::OhRenderer::next() {
    DBUG(Renderers);
    sendCommand("", "Next", constPlaylistService);
}

void Upnp::OhRenderer::stop() {
    DBUG(Renderers);
    sendCommand("", "Stop", constPlaylistService);
}

void Upnp::OhRenderer::seek(quint32 pos) {
    DBUG(Renderers) << pos;
    sendCommand(valueStr(pos), "SeekSecondAbsolute", constPlaylistService);
}

void Upnp::OhRenderer::setRepeat(bool r) {
    DBUG(Renderers) <<  r;
    sendCommand(valueStr(r ? 1 : 0), "SetRepeat", constPlaylistService);
}

void Upnp::OhRenderer::setShuffle(bool s) {
    DBUG(Renderers) << s;
    sendCommand(valueStr(s ? 1 : 0), "SetShuffle", constPlaylistService);
}

void Upnp::OhRenderer::clearQueue() {
    DBUG(Renderers);
    sendCommand("", "DeleteAll", constPlaylistService);
}

void Upnp::OhRenderer::mute(bool m) {
    DBUG(Renderers) << m;
    sendCommand("<InstanceID>0</InstanceID><Channel>Master</Channel><DesiredMute>"+QByteArray::number(m ? 1 : 0)+"</DesiredMute>",
                "SetMute", constRenderingControlService);
}

void Upnp::OhRenderer::setVolume(int vol) {
    DBUG(Renderers) << vol;
    sendCommand("<InstanceID>0</InstanceID><Channel>Master</Channel><DesiredVolume>"+QByteArray::number(vol)+"</DesiredVolume>",
                "SetVolume", constRenderingControlService);
}

void Upnp::OhRenderer::addTracks(Command *cmd) {
    DBUG(Renderers) << (void *)cmd << (void *)currentCmd << cmd->pos << cmd->type << cmd->tracks.count();
    if (currentCmd) {
        clearCommand();
    }
    currentCmd=cmd;
    if (Command::ReplaceAndPlay==cmd->type) {
        clearQueue();
    }
    if (Command::Move!=currentCmd->type) {
        emit info(tr("Adding tracks..."), Notif_PlayCommand);
    }
    quint32 after=0;
    if ((Command::Insert==cmd->type || Command::Move==cmd->type) && currentCmd->pos>=0 && currentCmd->pos<items.count()) {
        after=static_cast<Track *>(items.at(currentCmd->pos))->id;
    } else if (Command::Append==cmd->type && !items.isEmpty())  {
        after=static_cast<Track *>(items.at(items.count()-1))->id;
    }
    const MusicTrack *track=currentCmd->tracks.takeFirst();
    addTrack(track, after);
    delete track;
}

void Upnp::OhRenderer::addTrack(const MusicTrack *track, quint32 after) {
    QByteArray data;
    QXmlStreamWriter meta(&data);

    meta.writeStartDocument();
    meta.writeStartElement("DIDL-Lite");
    meta.writeDefaultNamespace("urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/");
    meta.writeNamespace("http://purl.org/dc/elements/1.1/", "dc");
    meta.writeNamespace("urn:schemas-upnp-org:metadata-1-0/upnp/", "upnp");
    meta.writeNamespace("urn:schemas-dlna-org:metadata-1-0/", "dlna");
    meta.writeStartElement("item");
//    xml.writeAttribute("id", "TODO");
    if (!track->name.isEmpty()) {
        meta.writeStartElement("dc:title");
        meta.writeCharacters(track->name);
        meta.writeEndElement();
    }
    if (!track->artist.isEmpty()) {
        meta.writeStartElement("dc:creator");
        meta.writeCharacters(track->artist);
        meta.writeEndElement();
        meta.writeStartElement("upnp:artist");
        meta.writeCharacters(track->artist);
        meta.writeEndElement();
    }
    if (!track->albumArtist.isEmpty()) {
        meta.writeStartElement("upnp:albumArtist");
        meta.writeCharacters(track->albumArtist);
        meta.writeEndElement();
    }
    if (!track->album.isEmpty()) {
        meta.writeStartElement("upnp:album");
        meta.writeCharacters(track->album);
        meta.writeEndElement();
    }
    if (!track->artUrl.isEmpty()) {
        meta.writeStartElement("upnp:albumArtURI");
        meta.writeCharacters(track->artUrl);
        meta.writeEndElement();
    }
    if (track->track>0) {
        meta.writeStartElement("upnp:originalTrackNumber");
        meta.writeCharacters(QString::number(track->track));
        meta.writeEndElement();
    }
    if (!track->date.isEmpty()) {
        meta.writeStartElement("dc:date");
        meta.writeCharacters(track->date);
        meta.writeEndElement();
    }
    if (!track->genre.isEmpty()) {
        meta.writeStartElement("upnp:genre");
        meta.writeCharacters(track->genre);
        meta.writeEndElement();
    }

    if (!track->url.isEmpty()) {
        meta.writeStartElement("res");
        QMap<QString, QString>::ConstIterator it=track->res.constBegin();
        QMap<QString, QString>::ConstIterator end=track->res.constEnd();
        for (; it!=end; ++it) {
            meta.writeAttribute(it.key(), it.value());
        }
        meta.writeCharacters(track->url);
        meta.writeEndElement();
    }
    meta.writeStartElement("upnp:class");
    meta.writeCharacters(track->isBroadcast ? constBroadcastClass : constTrackClass);
    meta.writeEndElement();

    meta.writeEndElement();
    meta.writeEndElement();
    meta.writeEndDocument();

    QByteArray msg;
    QXmlStreamWriter outer(&msg);
    outer.writeStartElement("AfterId");
    outer.writeCharacters(QString::number(after));
    outer.writeEndElement();
    outer.writeStartElement("Uri");
    outer.writeCharacters(track->url);
    outer.writeEndElement();
    outer.writeStartElement("Metadata");
    outer.writeCharacters(data);
    outer.writeEndElement();

    sendCommand(msg, "Insert", constPlaylistService);
}

void Upnp::OhRenderer::clearCommand() {
    delete currentCmd;
    currentCmd=0;
    addedCount=0;
}

void Upnp::OhRenderer::moveRows(const QList<quint32> &rows, qint32 to) {
    DBUG(Renderers) << rows << to;
    if (rows.isEmpty()) {
        return;
    }
    if (to>=0 && to<items.count()) {
        foreach (quint32 r, rows) {
            if (r>=items.count()) {
                return;
            }
        }
        // Convert 'to' row to an id
        if (to>0) {
            to=static_cast<Track *>(items.at(to-1))->id;
        }

        // No move command, so need to remove and re-add!
        if (rows.count()>1) {
            Command *cmd=new Command;
            cmd->pos=to;
            cmd->type=Command::Move;
            foreach (quint32 r, rows) {
                Track *track=static_cast<Track *>(items.at(r));
                sendCommand("<Value>"+QByteArray::number(track->id)+"</Value>", "DeleteId", constPlaylistService);
                cmd->tracks.append(new MusicTrack(*track));
            }
            addTracks(cmd);
        } else {
            Track *track=static_cast<Track *>(items.at(rows.first()));
            sendCommand("<Value>"+QByteArray::number(track->id)+"</Value>", "DeleteId", constPlaylistService);
            addTrack(track, to);
        }
    }
}

void Upnp::OhRenderer::removeTracks(const QModelIndexList &indexes) {
    foreach (const QModelIndex &idx, indexes) {
        DBUG(Renderers) << idx.data().toString();
        sendCommand("<Value>"+QByteArray::number(static_cast<Track *>(idx.internalPointer())->id)+"</Value>", "DeleteId", constPlaylistService);
    }
}

void Upnp::OhRenderer::play(const QModelIndex &idx) {
    DBUG(Renderers) << idx.data().toString();
    sendCommand("<Value>"+QByteArray::number(static_cast<Track *>(idx.internalPointer())->id)+"</Value>", "SeekId", constPlaylistService);
}

void Upnp::OhRenderer::emitAddedTracksNotif() {
    if (Command::Move!=currentCmd->type) {
        emit info(0==addedCount
                    ? tr("No tracks added")
                    : 1==addedCount
                      ? tr("1 track added")
                      : tr("%1 tracks added").arg(addedCount),
                Notif_PlayCommand, constNotifTimeout);
    }
}
