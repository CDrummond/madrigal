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

#include "upnp/renderers.h"
#include "upnp/ohrenderer.h"
#include "upnp/command.h"
#include "core/debug.h"

Upnp::Renderers::Renderers(HttpServer *h, QObject *parent)
    : DevicesModel("Upnp::Renderers", h, parent)
{
}

Upnp::Renderers::~Renderers() {
}

void Upnp::Renderers::addTracks(Upnp::Command *cmd) {
    DBUG(Renderers) << (void *)cmd;
    foreach (Device *dev, devices) {
        DBUG(Renderers) << (void *)dev << dev->isActive() << dev->uuid();
        if (dev->isActive()) {
            // Get active renderer to add tracks
            static_cast<Renderer *>(dev)->addTracks(cmd);
            return;
        }
    }

    // No active renderer?
    DBUG(Renderers) << "No active renderer";
    delete cmd;
}

Upnp::Device * Upnp::Renderers::createDevice(const Ssdp::Device &device) {
    if (device.services.contains(OhRenderer::constPlaylistService)) {
        OhRenderer *dev=new OhRenderer(device, this);
        DBUG(Renderers) << (void *)dev << dev->uuid();
        connect(dev, SIGNAL(acceptDrop(QByteArray,QList<QByteArray>,qint32)), this, SIGNAL(acceptDrop(QByteArray,QList<QByteArray>,qint32)));
        return dev;
    }
    // TODO: Non-openhome renderer?
    return 0;
}
