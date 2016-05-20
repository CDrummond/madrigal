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

#ifndef CORE_LYRICS_H
#define CORE_LYRICS_H

#include <QObject>
#include "upnp/renderer.h"

namespace Core {
class NetworkJob;

class Lyrics : public QObject {
    Q_OBJECT
public:
    Lyrics(QObject *p);
    virtual ~Lyrics() { cancel(); }

    void setEnabled(bool en);

Q_SIGNALS:
    void fetched(const QString &artist, const QString &title, const QString &text);

private Q_SLOTS:
    void setRenderer(const QModelIndex &idx);
    void update(const QModelIndex &idx);
    void jobFinished();

private:
    void setRenderer(Upnp::Renderer *r);
    void update(const Upnp::Device::MusicTrack &song);

private:
    void cancel();
    void searchResponse(NetworkJob *reply);
    void lyricsResponse(NetworkJob *reply);

private:
    bool enabled;
    NetworkJob *job;
    Upnp::Renderer *renderer;
    Upnp::Device::MusicTrack songWhenDisabled;
    Upnp::Device::MusicTrack currentSong;
};
}
#endif
