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
#include "core/debug.h"
#include "upnp/model.h"
#include "upnp/mediaservers.h"
#include "upnp/renderers.h"
#include <QTimer>

static const int constMaxTimeout=30;

Core::NotificationManager::NotificationManager(QObject *p)
    : QObject(p)
    , server(0)
    , renderer(0)
{
    timer=new QTimer(this);
    timer->setSingleShot(true);
    connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
    connect(Upnp::Model::self()->serversModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(activeServer(QModelIndex)));
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(activeRenderer(QModelIndex)));
}

Core::NotificationManager::~NotificationManager() {

}

void Core::NotificationManager::add(const QString &text, quint8 id, int timeout) {
    DBUG(Notifications) << text << id << timeout;
    Upnp::Device *dev=static_cast<Upnp::Device *>(sender());
    // Remove any previous notif with the same ID
    QList<QList<Notif>::iterator> toRemove;
    QList<Notif>::iterator it=notifs.begin();
    QList<Notif>::iterator end=notifs.end();

    for (; it!=end; ++it) {
        if ((it->id&0x00FF)==id) {
            toRemove.append(it);
        }
    }

    foreach (const QList<Notif>::iterator &i, toRemove) {
        notifs.erase(i);
    }

    // Add and send new notif
    if (timeout<0 || timeout>constMaxTimeout) {
        timeout=constMaxTimeout;
    }
    Notif notif(text, id+(renderer==dev ? 0x0100 : 0x0000));
    notif.expiry=QDateTime::currentDateTime();
    notif.expiry=notif.expiry.addSecs(timeout);
    if (!timer->isActive() || timer->remainingTime()>(timeout*1000)) {
        DBUG(Notifications) << "Start timer" << timeout;
        timer->start(timeout*1000);
    }
    DBUG(Notifications) << QString(notif.expiry.isValid() ? notif.expiry.toString() : "<>");

    notifs.append(notif);
    emit msg(text);
}

void Core::NotificationManager::timeout() {
    DBUG(Notifications) << "num notifs" << notifs.count();
    QList<QList<Notif>::iterator> toRemove;
    QList<Notif>::iterator it=notifs.begin();
    QList<Notif>::iterator end=notifs.end();
    QDateTime cur=QDateTime::currentDateTime();
    cur=cur.addMSecs(500);

    for (; it!=end; ++it) {
        if (it->expiry<cur) {
            toRemove.append(it);
        }
    }

    QString prev=notifs.isEmpty() ? QString() : notifs.last().text;
    foreach (const QList<Notif>::iterator &i, toRemove) {
        notifs.erase(i);
    }

    if (DBUG_ENABLED(Notifications)) {
        for (it=notifs.begin(); it!=end; ++it) {
            DBUG(Notifications) << " - " << it->id << it->text << QString(it->expiry.isValid() ? it->expiry.toString() : "<>");
        }
    }
    DBUG(Notifications) << "num notifs" << notifs.count() << "last" << QString(notifs.isEmpty() ? QString() : notifs.last().text) << "prev" << prev;
    if (notifs.isEmpty() || notifs.last().text!=prev) {
        emit msg(notifs.isEmpty() ? QString() : notifs.last().text);
    }

    if (!notifs.isEmpty()) {
        // Start the next timeout...
        int time=constMaxTimeout*1000;
        for (it=notifs.begin(); it!=end; ++it) {
            int ntime=QDateTime::currentDateTime().msecsTo(it->expiry);
            if (ntime<time) {
                time=ntime;
            }
        }
        DBUG(Notifications) << "Next timeout" << time/1000;
        timer->start(time);
    }
}

void Core::NotificationManager::activeServer(const QModelIndex &idx) {
    setActive(idx.isValid() ? static_cast<Upnp::Device *>(idx.internalPointer()) : 0, false);
}

void Core::NotificationManager::activeRenderer(const QModelIndex &idx) {
    setActive(idx.isValid() ? static_cast<Upnp::Device *>(idx.internalPointer()) : 0, true);
}

void Core::NotificationManager::cancel(Upnp::Device *dev) {
    if (notifs.isEmpty()) {
        return;
    }
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
