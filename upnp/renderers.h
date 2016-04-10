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

#ifndef UPNP_RENDERERS_H
#define UPNP_RENDERERS_H

#include "upnp/devicesmodel.h"
#include "upnp/device.h"

namespace Upnp {

class MediaServer;
struct Command;

class Renderers : public DevicesModel {
    Q_OBJECT

public:
    Renderers(HttpServer *h, QObject *parent = 0);
    virtual ~Renderers();

public Q_SLOTS:
    void addTracks(Upnp::Command *cmd);

Q_SIGNALS:
    void acceptDrop(const QByteArray &uuid, const QList<QByteArray> &ids, qint32 row);

private:
    Device * createDevice(const Ssdp::Device &device);
};

}

#endif
