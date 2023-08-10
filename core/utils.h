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

#ifndef CORE_UTILS_H
#define CORE_UTILS_H

#include "core/thread.h"
#include <math.h>
#include <sys/types.h>
#include <QtGlobal>
#include <QString>
#include <QLatin1Char>
#include <QDir>
#include <stdlib.h>
#ifdef Q_OS_WIN
#include <time.h>
#endif
#include <sys/time.h>

class QString;
class QUrl;
class QRectF;

namespace Core {
namespace Utils {
    extern const QLatin1Char constDirSep;
    extern const QLatin1String constDirSepStr;
    extern const char * constDirSepCharStr;

    inline bool equal(double d1, double d2, double precision=0.0001)
    {
        return (fabs(d1 - d2) < precision);
    }
    inline int random(int max=0) { return max ? (qrand()%max) : qrand(); }
#ifdef __MINGW32__
    inline void initRand() { timeval time; mingw_gettimeofday(&time, NULL); qsrand((time.tv_sec*1000)+(time.tv_usec/1000)); }
#else
    inline void initRand() { timeval time; gettimeofday(&time, NULL); qsrand((time.tv_sec*1000)+(time.tv_usec/1000)); }
#endif

    extern QString fixPath(const QString &d, bool ensureEndsInSlash=true);
    #ifdef Q_OS_WIN
    inline QString homeToTilda(const QString &s) { return s; }
    inline QString tildaToHome(const QString &s) { return s; }
    #else
    extern QString homeToTilda(const QString &s);
    extern QString tildaToHome(const QString &s);
    #endif

    extern QString getDir(const QString &file);
    extern QString getFile(const QString &file);
    extern QString changeExtension(const QString &file, const QString &extension);
    extern bool isDirReadable(const QString &dir);

    inline void msleep(int msecs) { Thread::msleep(msecs); }
    inline void sleep() { msleep(100); }

    extern QString strippedText(QString s);
    extern QString stripAcceleratorMarkers(QString label);

    // Convert path to a format suitable fo rUI - e.g. use native separators, and remove any trailing separator
    extern QString convertPathForDisplay(const QString &dir, bool isFolder=true);
    // Convert path from a UI field - convert to / separators, and add a trailing separator (if isFolder)
    extern QString convertPathFromDisplay(const QString &dir, bool isFolder=true);

    extern QString formatByteSize(double size);
    inline QString formatNumber(double number, int precision) { return QString::number(number, 'f', precision); }
    extern QString formatDuration(const quint32 totalseconds);
    extern QString formatTime(const quint32 seconds, bool zeroIsUnknown=false);

    extern QString cleanPath(const QString &p);
    extern QString dataDir(const QString &sub=QString(), bool create=false);
    extern QString cacheDir(const QString &sub=QString(), bool create=true);
    extern QString systemDir(const QString &sub);
    extern QString helper(const QString &app);
    extern bool moveFile(const QString &from, const QString &to);
    extern void moveDir(const QString &from, const QString &to);
    extern void clearOldCache(const QString &sub, int maxAge);
    extern void touchFile(const QString &fileName);
    extern void clearFolder(const QString &dir, const QStringList &fileTypes);
}

}

#endif
