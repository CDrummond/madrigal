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

#include "config.h"
#include "core/utils.h"
#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QDir>
#include <QProcess>
#include <QDateTime>
#include <QTime>
#include <QEventLoop>
#include <QStandardPaths>
#include <QLocale>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifndef Q_OS_WIN
#include <grp.h>
#include <pwd.h>
#endif
#include <sys/types.h>
#include <utime.h>

const QLatin1Char Core::Utils::constDirSep('/');
const QLatin1String Core::Utils::constDirSepStr("/");
const char * Core::Utils::constDirSepCharStr="/";

static const QLatin1String constHttp("http://");

QString Core::Utils::fixPath(const QString &dir, bool ensureEndsInSlash) {
    QString d(dir);

    if (!d.isEmpty() && !d.startsWith(constHttp)) {
        #ifdef Q_OS_WIN
        // Windows shares can be \\host\share (which gets converted to //host/share)
        // so if th epath starts with // we need to keep this double slash.
        bool startsWithDoubleSlash=d.length()>2 && d.startsWith(QLatin1String("//"));
        #endif
        d.replace(QLatin1String("//"), constDirSepStr);
        #ifdef Q_OS_WIN
        if (startsWithDoubleSlash) { // Re add first slash
            d=QLatin1Char('/')+d;
        }
        #endif
    }
    d.replace(QLatin1String("/./"), constDirSepStr);
    if (ensureEndsInSlash && !d.isEmpty() && !d.endsWith(constDirSep)) {
        d+=constDirSep;
    }
    return d;
}

#ifndef Q_OS_WIN
static const QLatin1String constTilda("~");
QString Core::Utils::homeToTilda(const QString &s) {
    QString hp=QDir::homePath();
    if (s==hp) {
        return constTilda;
    }
    if (s.startsWith(hp+constDirSepStr)) {
        return constTilda+fixPath(s.mid(hp.length()), false);
    }
    return s;
}

QString Core::Utils::tildaToHome(const QString &s) {
    if (s==constTilda) {
        return fixPath(QDir::homePath());
    }
    if (s.startsWith(constTilda+constDirSep)) {
        return fixPath(QDir::homePath()+constDirSepStr+s.mid(1), false);
    }
    return s;
}
#endif

QString Core::Utils::getDir(const QString &file) {
    bool isCueFile=file.contains("/cue:///") && file.contains("?pos=");
    QString d(file);
    int slashPos(d.lastIndexOf(constDirSep));

    if(slashPos!=-1) {
        d.remove(slashPos+1, d.length());
    }

    if (isCueFile) {
        d.remove("cue:///");
    }
    return fixPath(d);
}

QString Core::Utils::getFile(const QString &file) {
    QString d(file);
    int slashPos=d.lastIndexOf(constDirSep);

    if (-1!=slashPos) {
        d.remove(0, slashPos+1);
    }

    return d;
}

QString Core::Utils::changeExtension(const QString &file, const QString &extension) {
    if (extension.isEmpty()) {
        return file;
    }

    QString f(file);
    int pos=f.lastIndexOf('.');
    if (pos>1) {
        f=f.left(pos+1);
    }

    if (f.endsWith('.')) {
        return f+(extension.startsWith('.') ? extension.mid(1) : extension);
    }
    return f+(extension.startsWith('.') ? extension : (QChar('.')+extension));
}

bool Core::Utils::isDirReadable(const QString &dir) {
    #ifdef Q_OS_WIN
    if (dir.isEmpty()) {
        return false;
    } else {
        QDir d(dir);
        bool dirReadable=d.isReadable();
        // Handle cases where dir is set to \\server\ (i.e. no shared folder is set in path)
        if (!dirReadable && dir.startsWith(QLatin1String("//")) && d.isRoot() && (dir.length()-1)==dir.indexOf(Core::Utils::constDirSep, 2)) {
            dirReadable=true;
        }
        return dirReadable;
    }
    #else
    return dir.isEmpty() ? false : QDir(dir).isReadable();
    #endif
}

QString Core::Utils::strippedText(QString s) {
    s.remove(QString::fromLatin1("..."));
    int i = 0;
    while (i < s.size()) {
        ++i;
        if (s.at(i - 1) != QLatin1Char('&')) {
            continue;
        }

        if (i < s.size() && s.at(i) == QLatin1Char('&')) {
            ++i;
        }
        s.remove(i - 1, 1);
    }
    return s.trimmed();
}

QString Core::Utils::stripAcceleratorMarkers(QString label) {
    int p = 0;
    forever {
        p = label.indexOf('&', p);
        if(p < 0 || p + 1 >= label.length()) {
            break;
        }

        if(label.at(p + 1).isLetterOrNumber() || label.at(p + 1) == '&') {
            label.remove(p, 1);
        }

        ++p;
    }
    return label;
}

QString Core::Utils::convertPathForDisplay(const QString &path, bool isFolder) {
    if (path.isEmpty() || path.startsWith(constHttp)) {
        return path;
    }

    QString p(path);
    if (p.endsWith(constDirSep)) {
        p=p.left(p.length()-1);
    }
    /* TODO: Display ~/Music or /home/user/Music / /Users/user/Music ???
    p=homeToTilda(QDir::toNativeSeparators(p));
    */
    return QDir::toNativeSeparators(isFolder && p.endsWith(constDirSep) ? p.left(p.length()-1) : p);
}

QString Core::Utils::convertPathFromDisplay(const QString &path, bool isFolder) {
    QString p=path.trimmed();
    if (p.isEmpty()) {
        return p;
    }

    if (p.startsWith(constHttp)) {
        return fixPath(p);
    }
    return tildaToHome(fixPath(QDir::fromNativeSeparators(p), isFolder));
}

QString Core::Utils::formatByteSize(double size) {
    static bool useSiUnites=false;
    static QLocale locale;

    #ifndef Q_OS_WIN
    static bool init=false;
    if (!init) {
        init=true;
        const char *env=qgetenv("KDE_FULL_SESSION");
        QString dm=env && 0==strcmp(env, "true") ? QLatin1String("KDE") : QString(qgetenv("XDG_CURRENT_DESKTOP"));
        useSiUnites=!dm.isEmpty() && QLatin1String("KDE")!=dm;
    }
    #endif
    int unit = 0;
    double multiplier = useSiUnites ? 1000.0 : 1024.0;

    while (qAbs(size) >= multiplier && unit < 3) {
        size /= multiplier;
        unit++;
    }

    if (useSiUnites) {
        switch(unit) {
        case 0: return QObject::tr("%1 B").arg(size);
        case 1: return QObject::tr("%1 kB").arg(locale.toString(size, 'f', 1));
        case 2: return QObject::tr("%1 MB").arg(locale.toString(size, 'f', 1));
        default:
        case 3: return QObject::tr("%1 GB").arg(locale.toString(size, 'f', 1));
        }
    } else {
        switch(unit) {
        case 0: return QObject::tr("%1 B").arg(size);
        case 1: return QObject::tr("%1 KiB").arg(locale.toString(size, 'f', 1));
        case 2: return QObject::tr("%1 MiB").arg(locale.toString(size, 'f', 1));
        default:
        case 3: return QObject::tr("%1 GiB").arg(locale.toString(size, 'f', 1));
        }
    }
}

QString Core::Utils::formatDuration(const quint32 totalseconds) {
    //Get the days,hours,minutes and seconds out of the total seconds
    quint32 days = totalseconds / 86400;
    quint32 rest = totalseconds - (days * 86400);
    quint32 hours = rest / 3600;
    rest = rest - (hours * 3600);
    quint32 minutes = rest / 60;
    quint32 seconds = rest - (minutes * 60);

    //Convert hour,minutes and seconds to a QTime for easier parsing
    QTime time(hours, minutes, seconds);

    return 0==days
            ? time.toString("h:mm:ss")
            : QString("%1:%2").arg(days).arg(time.toString("hh:mm:ss"));
}

QString Core::Utils::formatTime(const quint32 seconds, bool zeroIsUnknown) {
    if (0==seconds && zeroIsUnknown) {
        return QObject::tr("Unknown");
    }

    static const quint32 constHour=60*60;
    if (seconds>constHour) {
        return Core::Utils::formatDuration(seconds);
    }

    QString result(QString::number(floor(seconds / 60.0))+QChar(':'));
    if (seconds % 60 < 10) {
        result += "0";
    }
    return result+QString::number(seconds % 60);
}

QString Core::Utils::cleanPath(const QString &p) {
    QString path(p);
    while (path.contains("//")) {
        path.replace("//", constDirSepStr);
    }
    return fixPath(path);
}

static QString userDir(const QString &mainDir, const QString &sub, bool create) {
    QString dir=mainDir;
    if (!sub.isEmpty()) {
        dir+=sub;
    }
    dir=Core::Utils::cleanPath(dir);
    QDir d(dir);
    return d.exists() || (create && d.mkpath(dir)) ? dir : QString();
}

QString Core::Utils::dataDir(const QString &sub, bool create) {
    #if defined Q_OS_WIN || defined Q_OS_MAC

    return userDir(QStandardPaths::writableLocation(QStandardPaths::DataLocation)+constDirSep, sub, create);

    #else

    static QString location;
    if (location.isEmpty()) {
        location=QStandardPaths::writableLocation(QStandardPaths::DataLocation);
        if (QCoreApplication::organizationName()==QCoreApplication::applicationName()) {
            location=location.replace(QCoreApplication::organizationName()+Core::Utils::constDirSep+QCoreApplication::applicationName(),
                                      QCoreApplication::applicationName());
        }
    }
    return userDir(location+constDirSep, sub, create);

    #endif
}

QString Core::Utils::cacheDir(const QString &sub, bool create) {
    #if defined Q_OS_WIN || defined Q_OS_MAC

    return userDir(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)+constDirSep, sub, create);

    #else

    static QString location;
    if (location.isEmpty()) {
        location=QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
        if (QCoreApplication::organizationName()==QCoreApplication::applicationName()) {
            location=location.replace(QCoreApplication::organizationName()+Core::Utils::constDirSep+QCoreApplication::applicationName(),
                                      QCoreApplication::applicationName());
        }
    }
    return userDir(location+constDirSep, sub, create);

    #endif
}

QString Core::Utils::systemDir(const QString &sub) {
    #if defined Q_OS_WIN
    return fixPath(QCoreApplication::applicationDirPath())+(sub.isEmpty() ? QString() : (sub+constDirSep));
    #elif defined Q_OS_MAC
    return fixPath(QCoreApplication::applicationDirPath())+QLatin1String("../Resources/")+(sub.isEmpty() ? QString() : (sub+constDirSep));
    #else
    return fixPath(QString(SHARE_INSTALL_PREFIX"/")+QCoreApplication::applicationName()+constDirSep+(sub.isEmpty() ? QString() : sub));
    #endif
}

QString Core::Utils::helper(const QString &app) {
    #if defined Q_OS_WIN
    return fixPath(QCoreApplication::applicationDirPath())+app+QLatin1String(".exe");
    #elif defined Q_OS_MAC
    return fixPath(QCoreApplication::applicationDirPath())+app;
    #else
    return QString(INSTALL_PREFIX "/"LINUX_LIB_DIR"/")+QCoreApplication::applicationName()+constDirSep+app;
    #endif
}

bool Core::Utils::moveFile(const QString &from, const QString &to) {
    return !from.isEmpty() && !to.isEmpty() && from!=to && QFile::exists(from) && !QFile::exists(to) && QFile::rename(from, to);
}

void Core::Utils::moveDir(const QString &from, const QString &to) {
    if (from.isEmpty() || to.isEmpty() || from==to) {
        return;
    }

    QDir f(from);
    if (!f.exists()) {
        return;
    }

    QDir t(to);
    if (!t.exists()) {
        return;
    }

    QFileInfoList files=f.entryInfoList(QStringList() << "*", QDir::Files|QDir::Dirs|QDir::NoDotAndDotDot);
    foreach (const QFileInfo &file, files) {
        if (file.isDir()) {
            QString dest=to+file.fileName()+constDirSep;
            if (!QDir(dest).exists()) {
                t.mkdir(file.fileName());
            }
            moveDir(from+file.fileName()+constDirSep, dest);
        } else {
            QFile::rename(from+file.fileName(), to+file.fileName());
        }
    }

    f.cdUp();
    f.rmdir(from);
}

void Core::Utils::clearOldCache(const QString &sub, int maxAge) {
    if (sub.isEmpty()) {
        return;
    }

    QString d=cacheDir(sub, false);
    if (d.isEmpty()) {
        return;
    }

    QDir dir(d);
    if (dir.exists()) {
        QFileInfoList files=dir.entryInfoList(QDir::Files|QDir::NoDotAndDotDot);
        if (files.count()) {
            QDateTime now=QDateTime::currentDateTime();
            foreach (const QFileInfo &f, files) {
                if (f.lastModified().daysTo(now)>maxAge) {
                    QFile::remove(f.absoluteFilePath());
                }
            }
        }
    }
}

void Core::Utils::touchFile(const QString &fileName) {
    ::utime(QFile::encodeName(fileName).constData(), 0);
}

static const int constMaxRecurseLevel = 4;
static void deleteAll(const QString &d, const QStringList &types, int level=0) {
    if (!d.isEmpty() && level<constMaxRecurseLevel) {
        QDir dir(d);
        if (dir.exists()) {
            QFileInfoList dirs=dir.entryInfoList(QDir::Dirs|QDir::NoDotAndDotDot);
            foreach (const QFileInfo &subDir, dirs) {
                deleteAll(subDir.absoluteFilePath(), types, level+1);
            }

            QFileInfoList files=dir.entryInfoList(types, QDir::Files|QDir::NoDotAndDotDot);
            foreach (const QFileInfo &file, files) {
                QFile::remove(file.absoluteFilePath());
            }
            if (0!=level) {
                QString dirName=dir.dirName();
                if (!dirName.isEmpty()) {
                    dir.cdUp();
                    dir.rmdir(dirName);
                }
            }
        }
    }
}

void Core::Utils::clearFolder(const QString &dir, const QStringList &fileTypes) {
    deleteAll(dir, fileTypes);
}
