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

quint16 Core::Debug::areas = 0;

void Core::Debug::setAreas(const QString &a) {
    QStringList items=a.split(",", QString::SkipEmptyParts);

    foreach(const QString &i, items) {
        if (QLatin1String("all")==i) {
            setAreas(All);
            break;
        } else if (QLatin1String("ssdp")==i) {
            setAreas(Ssdp);
        } else if (QLatin1String("mediaservers")==i) {
            setAreas(MediaServers);
        } else if (QLatin1String("renderers")==i) {
            setAreas(Renderers);
        } else if (QLatin1String("devices")==i) {
            setAreas(Devices);
        } else if (QLatin1String("http")==i) {
            setAreas(Http);
        } else if (QLatin1String("network")==i) {
            setAreas(Network);
        } else if (QLatin1String("thread")==i) {
            setAreas(Thread);
        } else if (QLatin1String("ui")==i) {
            setAreas(Ui);
        } else if (QLatin1String("images")==i) {
            setAreas(Images);
        } else if (QLatin1String("notifications")==i) {
            setAreas(Notifications);
        } else if (QLatin1String("lyrics")==i) {
            setAreas(Lyrics);
        }
    }
}
