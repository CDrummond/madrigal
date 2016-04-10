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

#include "upnp/httpserver.h"
#include "upnp/httpconnection.h"
#include "core/debug.h"
#include "core/networkaccessmanager.h"
#include "core/configuration.h"
#include <QUdpSocket>
#include <QThreadPool>

Upnp::HttpServer::HttpServer(QObject *p)
    : QTcpServer(p)
{
    // Try to use previous port...
    quint16 port=Core::Configuration(metaObject()->className()).get("port", 0, 0, 65535);
    if (0!=port && !listen(QHostAddress::Any, port)) {
        port=0;
    }

    if (!isListening() && !listen()) {
        qWarning() << "Could not start HTTP server";
        ::exit(0);
    }
    DBUG(Http) << "Started on port" << serverPort();
    pool=new QThreadPool(this);
    pool->setMaxThreadCount(10);
}

Upnp::HttpServer::~HttpServer() {
    Core::Configuration(metaObject()->className()).set("port", serverPort());
}

void Upnp::HttpServer::incomingConnection(qintptr handle) {
    HttpConnection *conn=new HttpConnection(handle, this);
    connect(conn, SIGNAL(notification(QByteArray,QByteArray)), SIGNAL(notification(QByteArray,QByteArray)));
}

QByteArray Upnp::HttpServer::getAddress(const QUrl &dest) {
    QString host=dest.host();
    QMap<QString, QByteArray>::ConstIterator it=addresses.find(host);
    if (it!=addresses.constEnd()) {
        return it.value();
    }

    QUdpSocket testSocket(this);
    testSocket.connectToHost(host, 1, QIODevice::ReadOnly);
    QByteArray address=testSocket.localAddress().toString().toLatin1();
    testSocket.close();
    addresses.insert(host, address);
    return address;
}
