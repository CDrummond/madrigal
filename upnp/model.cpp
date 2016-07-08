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

#include "core/debug.h"
#include "core/thread.h"
#include "core/globalstatic.h"
#include "core/networkaccessmanager.h"
#include "upnp/httpserver.h"
#include "upnp/mediaservers.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include "upnp/ssdp.h"
#include <QCommandLineParser>
#include <QThread>
#include <QTimer>

GLOBAL_STATIC(Upnp::Model, instance)

Upnp::Model::Model() {
    Core::Thread *thread=new Core::Thread("Ssdp", this);
    Core::NetworkAccessManager *network=new Core::NetworkAccessManager(0);
    ssdp=new Ssdp(0);
    http=new HttpServer(this);
    servers=new Upnp::MediaServers(http, this);
    renderers=new Upnp::Renderers(http, this);
    network->moveToThread(thread);
    ssdp->setNetwork(network);
    ssdp->moveToThread(thread);
    thread->start();
    connect(ssdp, SIGNAL(deviceAdded(Ssdp::Device)), servers, SLOT(added(Ssdp::Device)));
    connect(ssdp, SIGNAL(deviceRemoved(QByteArray)), servers, SLOT(removed(QByteArray)));
    connect(ssdp, SIGNAL(deviceAdded(Ssdp::Device)), renderers, SLOT(added(Ssdp::Device)));
    connect(ssdp, SIGNAL(deviceRemoved(QByteArray)), renderers, SLOT(removed(QByteArray)));
    connect(http, SIGNAL(notification(QByteArray,QByteArray,int)), servers, SLOT(notification(QByteArray,QByteArray,int)));
    connect(http, SIGNAL(notification(QByteArray,QByteArray,int)), renderers, SLOT(notification(QByteArray,QByteArray,int)));
    connect(thread, SIGNAL(started()), ssdp, SLOT(start()));
    connect(servers, SIGNAL(addTracks(Upnp::Command*)), renderers, SLOT(addTracks(Upnp::Command*)));
    connect(renderers, SIGNAL(acceptDrop(QByteArray,QList<QByteArray>,qint32)), servers, SLOT(play(QByteArray,QList<QByteArray>,qint32)));
    connect(qApp, SIGNAL(aboutToQuit()), this, SLOT(clear()));
}

void Upnp::Model::discoverDevices() {
    QMetaObject::invokeMethod(ssdp, "search", Qt::QueuedConnection);
}

void Upnp::Model::clear() {
    delete servers;
    delete renderers;
    delete http;
    delete ssdp;
    servers=0;
    renderers=0;
    http=0;
    ssdp=0;
}
