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

#ifndef UPNP_RENDERER_H
#define UPNP_RENDERER_H

#include "upnp/device.h"

namespace Upnp {

struct Command;
// TODO: Standard UPnp renderer - needs to manually control playqueue
class Renderer : public Device {
    Q_OBJECT
public:
    struct Volume {
        Volume() : max(100), steps(100), current(0), muted(false) { }
        quint32 max;
        quint32 steps;
        quint32 current;
        bool muted;
    };

    enum State {
        Null,
        Stopped,
        Playing,
        Paused
    };

    struct Playback {
        Playback() : state(Null), seconds(0), duration(0), shuffle(false), repeat(false) { }
        State state;
        quint32 seconds;
        quint32 duration;
        bool shuffle;
        bool repeat;
    };

    struct Track : public MusicTrack {
        Track(quint32 i, const QMap<QString, QString> &values, Item *p=0, int r=0)
            : MusicTrack(values, p, r), id(i) { }
        Track(quint32 i=0, const QString &n=QString(), Item *p=0, int r=0)
            : MusicTrack(n, p, r), id(i) { }
        virtual ~Track() { }
        virtual QString subText() const;
        virtual QString otherText() const { return 0==duration ? QString() : Core::Utils::formatTime(duration); }
        quint32 id;
    };

public:
    Renderer(const Ssdp::Device &device, DevicesModel *parent)
        : Device (device, parent), pingTimer(0), currentTrackId(0) { }
    virtual ~Renderer() { }
    virtual void setActive(bool a);
    virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::no_icon==details.icon ? Core::MonoIcon::volumeup : details.icon; }
    QVariant data(const QModelIndex &index, int role) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    virtual QModelIndex current() = 0;
    const Volume & volume() const { return volState; }
    const Playback playback() const { return playState; }
    Qt::DropActions supportedDropActions() const;
    QStringList mimeTypes() const;
    QMimeData *mimeData(const QModelIndexList &indexes) const;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);
    QByteArray toXml() const;

public Q_SLOTS:
    virtual void previous() = 0;
    virtual void playPause() = 0;
    virtual void next() = 0;
    virtual void stop() = 0;
    virtual void seek(quint32 pos) = 0;
    virtual void setRepeat(bool r) = 0;
    virtual void setShuffle(bool s) = 0;
    virtual void clearQueue() = 0;
    virtual void mute(bool m) = 0;
    virtual void setVolume(int vol) = 0;
    virtual void addTracks(Upnp::Command *cmd) = 0;
    virtual void removeTracks(const QModelIndexList &indexes) = 0;
    virtual void play(const QModelIndex &idx) = 0;

protected:
    void messageReceived();

private Q_SLOTS:
    virtual void sendPing() { }

private:
    virtual void moveRows(const QList<quint32> &rows, qint32 to) = 0;

Q_SIGNALS:
    void currentTrack(const QModelIndex &idx);
    void playbackPos(quint32 pos);
    void playbackDuration(quint32 total);
    void playbackState(Upnp::Renderer::State state);
    void volumeState(const Upnp::Renderer::Volume &vol);
    void repeat(bool on);
    void shuffle(bool on);
    void queueDetails(quint32 numTracks, quint32 duration);
    void acceptDrop(const QByteArray &uuid, const QList<QByteArray> &ids, qint32 row);

protected:
    void updateStats();

protected:
    QTimer *pingTimer;
    Volume volState;
    Playback playState;
    quint32 currentTrackId;
};

}

#endif
