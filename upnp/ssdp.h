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

#ifndef UPNP_SSDP_H
#define UPNP_SSDP_H

#include "core/monoicon.h"
#include <QObject>
#include <QList>
#include <QMap>
#include <QSet>

class QUdpSocket;
class QTimer;
namespace Core {
class NetworkAccessManager;
class NetworkJob;
}

namespace Upnp {

class Ssdp : public QObject {
    Q_OBJECT
public:
    struct Device {
        struct Service {
            QByteArray id;
            QByteArray controlUrl;
            QByteArray eventUrl;
        };
        typedef QMap<QByteArray, Service> Services;

        QByteArray type;
        QByteArray uuid;
        QString manufacturer;
        QString name;
        QByteArray baseUrl;
        QString host;
        Services services;
        Core::MonoIcon::Type icon;
    };

public:
    Ssdp(QObject *p);
    void setNetwork(Core::NetworkAccessManager *net) { if (!network) network=net; }

Q_SIGNALS:
    void deviceAdded(const Ssdp::Device &details);
    void deviceRemoved(const QByteArray &uuid);

public Q_SLOTS:
    void start();

private Q_SLOTS:
    void search();
    void readDatagrams();
    void jobFinished();
    void listingTimeout();

private:
    Core::NetworkAccessManager *network;
    QUdpSocket *socket;
    QList<Core::NetworkJob *> jobs;
    QSet<QByteArray> knownDevices;
    QSet<QByteArray> listedDevices;
    QTimer *refreshTimer;
    QTimer *listTimer;
};

}

#endif
