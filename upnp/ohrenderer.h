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

#ifndef UPNP_OH_RENDERER_H
#define UPNP_OH_RENDERER_H

#include "upnp/renderer.h"
#include <QSet>

namespace Upnp {

class OhRenderer : public Renderer {
    Q_OBJECT

public:
    struct Source {
        Source(const QString &n=QString(), const QString &t=QString(), bool v=false)
            : name(n), type(t), visible(v) { }
        bool operator ==(const Source &o) const { return visible==o.visible && name==o.name && type==o.type; }
        QString name;
        QString type;
        bool visible;
    };

    static const char * constPlaylistService;
    static const char * constRadioService;
    static const char * constReceiverService;
    static const char * constSenderService;

    OhRenderer(const Ssdp::Device &device, DevicesModel *parent);
    virtual ~OhRenderer();

public Q_SLOTS:
    void selectSource(const QString &src);

private:
    void setActive(bool a);
    void clear();
    void populate();
    void sendPing();
    void commandResponse(QXmlStreamReader &reader, const QByteArray &type, Core::NetworkJob *job);
    void failedCommand(Core::NetworkJob *job, const QByteArray &type);
    void notification(const QByteArray &sid, const QByteArray &data);
    void updateTransportState(const QString &val);
    void updateCurrentTrackId(const QString &val);
    void updateShuffle(const QString &val);
    void updateRepeat(const QString &val);
    void handleIdArray(QXmlStreamReader &reader);
    void handleInsert(QXmlStreamReader &reader);
    void handleSourceIndex(quint32 val);
    void handleSourceXml(QXmlStreamReader &reader);
    QString getValue(QXmlStreamReader &reader);
    void handleReadList(QXmlStreamReader &reader);
    qint32 getRowById(quint32 id) const;
    void updateTracks(const QList<quint32> &update);
    QMap<QString, QString> parseTrackMetadata(const QString &xml);
    void updateCurrentTrackId(quint32 id);
    QModelIndex current();
    void previous();
    void playPause();
    void next();
    void stop();
    void seek(quint32 pos);
    void setRepeat(bool r);
    void setShuffle(bool s);
    void clearQueue();
    void mute(bool m);
    void setVolume(int vol);
    void addTracks(Upnp::Command *cmd);
    void addTrack(const MusicTrack *track, quint32 after);
    void clearCommand();
    void moveRows(const QList<quint32> &rows, qint32 to);
    void removeTracks(const QModelIndexList &indexes);
    void play(const QModelIndex &idx);
    void emitAddedTracksNotif();

private:
    Command *currentCmd;
    int addedCount;
    QSet<quint32> ids;
    qint32 sourceIndex;
    QList<Source> sources;
};

}

#endif
