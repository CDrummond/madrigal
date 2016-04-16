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

#include "mac/notify.h"
#include "mac/notification.h"
#include "core/images.h"
#include "core/configuration.h"
#include "core/globalstatic.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include "config.h"
#include <QCoreApplication>
#include <QImage>

GLOBAL_STATIC(Mac::Notify, instance)

Mac::Notify::Notify(QObject *p)
    : QObject(p)
    , renderer(0)
{
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(setRenderer(QModelIndex)));
    enabled=Core::Configuration(this).get("enabled", true);
}

void Mac::Notify::setEnabled(bool en) {
    if (en!=enabled) {
        enabled=en;
        Core::Configuration(this).set("enabled", enabled);
    }
}

void Mac::Notify::setRenderer(Upnp::Renderer *r) {
    if (r==renderer) {
        return;
    }
    if (renderer) {
        disconnect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
    }
    renderer=r;
    if (renderer) {
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
        update(renderer->current());
    } else {
        update(QModelIndex());
    }
}

void Mac::Notify::setRenderer(const QModelIndex &idx) {
    setRenderer(static_cast<Upnp::Renderer *>(idx.internalPointer()));
}

void Mac::Notify::update(const QModelIndex &idx) {
    if (idx.isValid()) {
        update(*static_cast<Upnp::Device::MusicTrack *>(idx.internalPointer()));
    } else {
        update(Upnp::Device::MusicTrack());
    }
}

void Mac::Notify::update(const Upnp::Device::MusicTrack &song)
{
    if (song.artistName()!=currentSong.artistName() || song.album!=currentSong.album ||
        song.track!=currentSong.track || song.name!=currentSong.name) {
        currentSong = song;
        if (!currentSong.artist.isEmpty() && enabled) {
            QImage *i=Core::Images::self()->get(Core::ImageDetails(song.artUrl, song.artistName(), song.album), 0, true);
            showNotification(tr("Now playing"), song.describe(), i ? *i : QImage());
        }
    }
}
