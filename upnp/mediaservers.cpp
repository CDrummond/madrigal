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

#include "upnp/mediaservers.h"
#include "upnp/mediaserver.h"
#include "upnp/localplaylists.h"
#include "core/debug.h"

Upnp::MediaServers::MediaServers(HttpServer *h, QObject *parent)
    : DevicesModel("Upnp::MediaServers", h, parent)
{
    connect(LocalPlaylists::self(), SIGNAL(addTracks(Upnp::Command*)), SIGNAL(addTracks(Upnp::Command*)));
    beginInsertRows(QModelIndex(), devices.size(), devices.size());
    devices.append(LocalPlaylists::self());
    endInsertRows();
}

Upnp::MediaServers::~MediaServers() {
}

void Upnp::MediaServers::play(const QByteArray &uuid, const QList<QByteArray> &ids, qint32 row) {
    DBUG(MediaServers) << uuid << ids << row;
    foreach (Device *device, devices) {
        if (device->uuid()==uuid) {
            static_cast<MediaServer *>(device)->play(ids, row);
        }
    }
}

Upnp::Device * Upnp::MediaServers::createDevice(const Ssdp::Device &device) {
    if (device.services.contains(Upnp::MediaServer::constContentDirService)) {
        MediaServer *server=new MediaServer(device, this);
        connect(server, SIGNAL(addTracks(Upnp::Command*)), SIGNAL(addTracks(Upnp::Command*)));
        return server;
    }
    return 0;
}
