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

#ifndef DBUS_GNOME_MEDIA_KEYS_H
#define DBUS_GNOME_MEDIA_KEYS_H

#include "core/mediakeys.h"

class OrgGnomeSettingsDaemonInterface;
class OrgGnomeSettingsDaemonMediaKeysInterface;
class QDBusPendingCallWatcher;
class QDBusServiceWatcher;

namespace Dbus {
class GnomeMediaKeys : public Core::MediaKeys {
    Q_OBJECT

public:
    GnomeMediaKeys(QObject *p);

    bool activate();
    void deactivate();

private:
    bool daemonIsRunning();
    void releaseKeys();
    void grabKeys();
    void disconnectDaemon();

private Q_SLOTS:
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void registerFinished(QDBusPendingCallWatcher *watcher);
    void keyPressed(const QString &app, const QString &key);
    void pluginActivated(const QString &name);

private:
    OrgGnomeSettingsDaemonInterface *daemon;
    OrgGnomeSettingsDaemonMediaKeysInterface *mk;
    QDBusServiceWatcher *watcher;
};

}

#endif
