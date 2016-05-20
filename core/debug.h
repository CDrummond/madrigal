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

#ifndef CORE_DEBUG_H
#define CORE_DEBUG_H

#include <QDebug>
#include <QDateTime>
#include <QThread>

namespace Core {
class Debug {
public:
    enum Areas {
        Ssdp          = 0x0001,
        MediaServers  = 0x0002,
        Renderers     = 0x0004,
          Devices     = MediaServers | Renderers,
        Http          = 0x0008,
        Network       = 0x0010,
        Thread        = 0x0020,
        Ui            = 0x0040,
        Images        = 0x0080,
        Notifications = 0x0100,
        Lyrics        = 0x0200,

        All           = 0xFFFF
    };

    static bool isEnabled(quint16 area) { return areas&area; }
    static void setAreas(quint16 a) { areas|=a; }
    static void setAreas(const QString &a);
private:
    static quint16 areas;
};

}

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)
#define DBUG_ENABLED(AREA) Core::Debug::isEnabled(Core::Debug::AREA)
#define DBUG(AREA) if (DBUG_ENABLED(AREA)) \
    qWarning() << QDateTime::currentDateTime().toString(Qt::ISODate) \
               << QThread::currentThread()->objectName() \
               << metaObject()->className() \
               << __FUNCTION__ \
               << __LINE__ \
               << (void *)this

#define DBUGF(AREA) if (DBUG_ENABLED(AREA)) \
    qWarning() << QDateTime::currentDateTime().toString(Qt::ISODate) \
               << QThread::currentThread()->objectName() \
               << __FILENAME__ \
               << __FUNCTION__ \
               << __LINE__
#endif
