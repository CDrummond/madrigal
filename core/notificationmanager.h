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

#ifndef UPNP_NOTIFICATION_MANAGER_H
#define UPNP_NOTIFICATION_MANAGER_H

#include <QObject>
#include <QSet>
#include <QDateTime>

#include "upnp/device.h"

class QTimer;

namespace Core {

class NotificationManager : public QObject {
    Q_OBJECT
    struct Notif {
        Notif(const QString &t=QString(), quint16 i=0)
            : text(t), id(i) { }
        QString text;
        quint16 id;
        QDateTime expiry;
    };

public:
    NotificationManager(QObject *p);
    virtual ~NotificationManager();

Q_SIGNALS:
    void msg(const QString &text);

private Q_SLOTS:
    void add(const QString &text, quint8 id, int timeout);
    void timeout();
    void activeServer(const QModelIndex &idx);
    void activeRenderer(const QModelIndex &idx);

private:
    void cancel(Upnp::Device *dev);
    void setActive(Upnp::Device *dev, bool isRenderer);

private:
    Upnp::Device *server;
    Upnp::Device *renderer;
    QTimer *timer;
    QList<Notif> notifs;
};

}

#endif
