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

#include "core/notificationmanager.h"
#include "upnp/model.h"
#include "upnp/mediaservers.h"
#include "upnp/renderers.h"
#include <QTimer>

Core::NotificationManager::NotificationManager(QObject *p)
    : QObject(p)
    , server(0)
    , renderer(0)
{
    timer=new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    connect(Upnp::Model::self()->serversModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(activeServer(QModelIndex)));
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(activeRenderer(QModelIndex)));
}

Core::NotificationManager::~NotificationManager() {

}

void Core::NotificationManager::add(const QString &text, quint8 id, int timeout) {
    Upnp::Device *dev=static_cast<Upnp::Device *>(sender());
    quint16 msgid=(dev==renderer ? 0x0100 : 0x0000)+id;
    // Remove any previous notif with the same ID
    QList<QList<Notif>::iterator> toRemove;
    QList<Notif>::iterator it=notifs.begin();
    QList<Notif>::iterator end=notifs.end();

    for (; it!=end; ++it) {
        if (it->id==msgid) {
            toRemove.append(it);
        }
    }

    foreach (const QList<Notif>::iterator &i, toRemove) {
        notifs.erase(i);
    }

    // Add and send new notif
    Notif notif(text, msgid);
    if (timeout>0) {
        notif.expiry=QDateTime::currentDateTime();
        notif.expiry.addSecs(timeout);
        if (!timer->isActive() || timer->remainingTime()>(timeout*1000)) {
            timer->start(timeout*1000);
        }
    }

    notifs.append(notif);
    emit msg(text);
}

void Core::NotificationManager::timeout() {
    QList<QList<Notif>::iterator> toRemove;
    QList<Notif>::iterator it=notifs.begin();
    QList<Notif>::iterator end=notifs.end();
    QDateTime cur=QDateTime::currentDateTime();
    cur.addMSecs(500);

    for (; it!=end; ++it) {
        if (it->expiry<cur) {
            toRemove.append(it);
        }
    }

    QString prev=notifs.isEmpty() ? QString() : notifs.last().text;
    foreach (const QList<Notif>::iterator &i, toRemove) {
        notifs.erase(i);
    }

    if (notifs.isEmpty() || notifs.last().text!=prev) {
        emit msg(notifs.isEmpty() ? QString() : notifs.last().text);
    }
}

void Core::NotificationManager::activeServer(const QModelIndex &idx) {
    setActive(idx.isValid() ? static_cast<Upnp::Device *>(idx.internalPointer()) : 0, false);
}

void Core::NotificationManager::activeRenderer(const QModelIndex &idx) {
    setActive(idx.isValid() ? static_cast<Upnp::Device *>(idx.internalPointer()) : 0, true);
}

void Core::NotificationManager::cancel(Upnp::Device *dev) {
    quint16 devid=dev==renderer ? 0x0100 : 0x0000;
    bool sendNew=(notifs.last().id&0xFF00)==devid;

    QList<QList<Notif>::iterator> toRemove;
    QList<Notif>::iterator it=notifs.begin();
    QList<Notif>::iterator end=notifs.end();

    for (; it!=end; ++it) {
        if ((it->id&0xFF00)==devid) {
            toRemove.append(it);
        }
    }

    foreach (const QList<Notif>::iterator &i, toRemove) {
        notifs.erase(i);
    }

    if (sendNew) {
        emit msg(notifs.isEmpty() ? QString() : notifs.last().text);
    }
}

void Core::NotificationManager::setActive(Upnp::Device *dev, bool isRenderer) {
    Upnp::Device *prev=isRenderer ? renderer : server;
    if (prev) {
        cancel(prev);
        disconnect(prev, SIGNAL(info(QString,quint8,int)), this, SLOT(add(QString,quint8,int)));
    }
    if (dev) {
        connect(dev, SIGNAL(info(QString,quint8,int)), this, SLOT(add(QString,quint8,int)));
        if (isRenderer) {
            renderer=dev;
        } else {
            server=dev;
        }
    }
}
