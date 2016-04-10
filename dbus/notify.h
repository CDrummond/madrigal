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

#ifndef DBUS_NOTIFY_H
#define DBUS_NOTIFY_H

#include <QObject>
#include <QDateTime>
#include "upnp/renderer.h"

class QImage;
class QDBusPendingCallWatcher;
class QDBusArgument;
class OrgFreedesktopNotificationsInterface;

QDBusArgument& operator<< (QDBusArgument &arg, const QImage &image);
const QDBusArgument& operator>> (const QDBusArgument &arg, QImage &image);

namespace Dbus {
class Notify : public QObject {
    Q_OBJECT

public:
    static Notify * self();

    Notify(QObject *p=0);
    virtual ~Notify() { }

    void show(const QString &title, const QString &text, const QImage &img);
    bool isEnabled() const { return enabled; }
    void setEnabled(bool en);
    
private Q_SLOTS:
    void callFinished(QDBusPendingCallWatcher *watcher);
    void setRenderer(const QModelIndex &idx);
    void update(const QModelIndex &idx);

private:
    void setRenderer(Upnp::Renderer *r);
    void update(const Upnp::Device::MusicTrack &song);

private:
    bool enabled;
    QDateTime lastTime;
    int lastId;
    OrgFreedesktopNotificationsInterface *iface;
    Upnp::Renderer *renderer;
    Upnp::Device::MusicTrack currentSong;
};

}
#endif
