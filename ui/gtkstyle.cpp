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

#include "ui/gtkstyle.h"
#include "ui/proxystyle.h"
#include "ui/utils.h"
#include "core/utils.h"
#include "config.h"
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QCache>
#include <QProcess>
#include <QTextStream>
#include <QFile>
#include <qglobal.h>

#if defined Q_OS_WIN || defined Q_OS_MAC || defined QT_NO_STYLE_GTK
#define NO_GTK_SUPPORT
#endif

#ifndef NO_GTK_SUPPORT
#include "ui/gtkproxystyle.h"
#include "ui/windowmanager.h"
#if QT_VERSION < 0x050000
#include <QGtkStyle>
#endif
#endif

static bool usingGtkStyle=false;

bool Ui::GtkStyle::isActive() {
    #ifndef NO_GTK_SUPPORT
    static bool init=false;
    if (!init) {
        init=true;
        usingGtkStyle=QApplication::style()->inherits("QGtkStyle");
        if (usingGtkStyle) {
            QCoreApplication::setAttribute(Qt::AA_DontShowIconsInMenus);
        }
    }
    #endif
    return usingGtkStyle;
}

void Ui::GtkStyle::drawSelection(const QStyleOptionViewItem &opt, QPainter *painter, double opacity) {
    static const int constMaxDimension=32;
    static QCache<QString, QPixmap> cache(30000);

    if (opt.rect.width()<2 || opt.rect.height()<2) {
        return;
    }

    int width=qMin(constMaxDimension, opt.rect.width());
    QString key=QString::number(width)+QChar(':')+QString::number(opt.rect.height());
    QPixmap *pix=cache.object(key);

    if (!pix) {
        pix=new QPixmap(width, opt.rect.height());
        QStyleOptionViewItem styleOpt(opt);
        pix->fill(Qt::transparent);
        QPainter p(pix);
        styleOpt.state=opt.state;
        styleOpt.state&=~(QStyle::State_Selected|QStyle::State_MouseOver);
        styleOpt.state|=QStyle::State_Selected|QStyle::State_Enabled|QStyle::State_Active;
        styleOpt.viewItemPosition = QStyleOptionViewItem::OnlyOne;
        styleOpt.rect=QRect(0, 0, opt.rect.width(), opt.rect.height());
        QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &styleOpt, &p, 0);
        p.end();
        cache.insert(key, pix, pix->width()*pix->height());
    }
    double opacityB4=painter->opacity();
    painter->setOpacity(opacity);
    if (opt.rect.width()>pix->width()) {
        int half=qMin(opt.rect.width()>>1, pix->width()>>1);
        painter->drawPixmap(opt.rect.x(), opt.rect.y(), pix->copy(0, 0, half, pix->height()));
        if ((half*2)!=opt.rect.width()) {
            painter->drawTiledPixmap(opt.rect.x()+half, opt.rect.y(), (opt.rect.width()-((2*half))), opt.rect.height(), pix->copy(half-1, 0, 1, pix->height()));
        }
        painter->drawPixmap((opt.rect.x()+opt.rect.width())-half, opt.rect.y(), pix->copy(half, 0, half, pix->height()));
    } else {
        painter->drawPixmap(opt.rect, *pix);
    }
    painter->setOpacity(opacityB4);
}

QString Ui::GtkStyle::readDconfSetting(const QString &setting, const QString &scheme) {
    #ifdef NO_GTK_SUPPORT
    Q_UNUSED(setting)
    Q_UNUSED(scheme)
    #else
    // For some reason, dconf does not seem to terminate correctly when run under some desktops (e.g. KDE)
    // Destroying the QProcess seems to block, causing the app to appear to hang before starting.
    // So, create QProcess on the heap - and only wait 1.5s for response. Connect finished to deleteLater
    // so that the object is destroyed.
    static bool first=true;
    static bool ok=true;

    if (first) {
        first=false;
    } else if (!ok) { // Failed before, so dont bothe rcalling dconf again!
        return QString();
    }

    QString schemeToUse=scheme.isEmpty() ? QLatin1String("/org/gnome/desktop/interface/") : scheme;
    QProcess *process=new QProcess();
    process->start(QLatin1String("dconf"), QStringList() << QLatin1String("read") << schemeToUse+setting);
    QObject::connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));

    if (process->waitForFinished(1500)) {
        QString resp = process->readAllStandardOutput();
        resp = resp.trimmed();
        resp.remove('\'');

        if (resp.isEmpty()) {
            // Probably set to the default, and dconf does not store defaults! Therefore, need to read via gsettings...
            schemeToUse=schemeToUse.mid(1, schemeToUse.length()-2).replace("/", ".");
            QProcess *gsettingsProc=new QProcess();
            gsettingsProc->start(QLatin1String("gsettings"), QStringList() << QLatin1String("get") << schemeToUse << setting);
            QObject::connect(gsettingsProc, SIGNAL(finished(int)), process, SLOT(deleteLater()));
            if (gsettingsProc->waitForFinished(1500)) {
                resp = gsettingsProc->readAllStandardOutput();
                resp = resp.trimmed();
                resp.remove('\'');
            } else {
                gsettingsProc->kill();
            }
        }
        return resp;
    } else { // If we failed 1 time, dont bother next time!
        ok=false;
    }
    process->kill();
    #endif
    return QString();
}

#ifndef NO_GTK_SUPPORT
static QString themeNameSetting;
#endif

// Copied from musique
QString Ui::GtkStyle::themeName() {
    #ifdef NO_GTK_SUPPORT
    return QString();
    #else

    if (themeNameSetting.isEmpty()) {
        static bool read=false;

        if (read) {
            return themeNameSetting;
        }
        read=true;

        if (themeNameSetting.isEmpty()) {
            themeNameSetting=readDconfSetting(QLatin1String("gtk-theme"));
            if (!themeNameSetting.isEmpty()) {
                return themeNameSetting;
            }

            QString rcPaths = QString::fromLocal8Bit(qgetenv("GTK2_RC_FILES"));
            if (!rcPaths.isEmpty()) {
                QStringList paths = rcPaths.split(QLatin1String(":"));
                foreach (const QString &rcPath, paths) {
                    if (!rcPath.isEmpty()) {
                        QFile rcFile(rcPath);
                        if (rcFile.exists() && rcFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
                            QTextStream in(&rcFile);
                            while(!in.atEnd()) {
                                QString line = in.readLine();
                                if (line.contains(QLatin1String("gtk-theme-name"))) {
                                    line = line.right(line.length() - line.indexOf(QLatin1Char('=')) - 1);
                                    line.remove(QLatin1Char('\"'));
                                    line = line.trimmed();
                                    themeNameSetting = line;
                                    break;
                                }
                            }
                        }
                    }
                    if (!themeNameSetting.isEmpty()) {
                        break;
                    }
                }
            }

            #if QT_VERSION < 0x050000
            // Fall back to gconf
            if (themeNameSetting.isEmpty()) {
                themeNameSetting = QUi::GtkStyle::getGConfString(QLatin1String("/desktop/gnome/interface/gtk_theme"));
            }
            #endif
            if (themeNameSetting.isEmpty() && Ui::Utils::Unity==Ui::Utils::currentDe()) {
                themeNameSetting=QLatin1String("Ambiance");
            }
        }
    }
    return themeNameSetting;
    #endif
}

extern void Ui::GtkStyle::setThemeName(const QString &n) {
    #ifdef NO_GTK_SUPPORT
    Q_UNUSED(n)
    #else
    themeNameSetting=n;
    #endif
}

#ifndef NO_GTK_SUPPORT
static Ui::WindowManager *wm=0;
#endif
static QProxyStyle *proxyStyle=0;

void Ui::GtkStyle::applyTheme(QWidget *widget) {
    #ifdef NO_GTK_SUPPORT
    Q_UNUSED(widget)
    #else
    if (widget && isActive()) {
        QString theme=Ui::GtkStyle::themeName().toLower();
        int modViewFrame=0;
        QMap<QString, QString> css;
        if (!theme.isEmpty()) {
            QFile cssFile(Core::Utils::systemDir(QLatin1String("themes"))+theme+QLatin1String(".css"));
            if (cssFile.open(QFile::ReadOnly|QFile::Text)) {
                while (!cssFile.atEnd()) {
                    QString line = cssFile.readLine().trimmed();
                    if (line.isEmpty()) {
                        continue;
                    }
                    if (line.startsWith(QLatin1String("/*"))) {
                        if (!wm && line.contains("drag:toolbar")) {
                            wm=new WindowManager(widget);
                            wm->registerWidgetAndChildren(widget);
                            QMainWindow *win=qobject_cast<QMainWindow *>(widget->window());
                            if (win && win->menuBar()) {
                                wm->registerWidgetAndChildren(win->menuBar());
                            }
                        }
                        modViewFrame=line.contains("modview:ts")
                                        ? ProxyStyle::VF_Side|ProxyStyle::VF_Top
                                        : line.contains("modview:true")
                                            ? ProxyStyle::VF_Side
                                            : 0;
                    } else {
                        int space=line.indexOf(' ');
                        if (space>2) {
                            css.insert(line.left(space), line);
                        }
                    }
                }
            }
        }
        if (!proxyStyle) {
            proxyStyle=new GtkProxyStyle(modViewFrame, true, true, css);
        }
    }
    #endif

    if (!proxyStyle) {
        #if defined Q_OS_WIN
        int modViewFrame=ProxyStyle::VF_Side;
        bool controlMnemonics=true;
        #elif defined Q_OS_MAC
        int modViewFrame=ProxyStyle::VF_Side|ProxyStyle::VF_Top;
        bool controlMnemonics=true;
        #else
        int modViewFrame=qApp->style() && qApp->style()->inherits("QFusionStyle") ? ProxyStyle::VF_Side : 0;
        bool controlMnemonics=0!=modViewFrame;
        #endif

        if (modViewFrame || controlMnemonics) {
            proxyStyle=new ProxyStyle(modViewFrame, controlMnemonics);
        }
    }
    if (proxyStyle) {
        qApp->setStyle(proxyStyle);
    }
}

void Ui::GtkStyle::registerWidget(QWidget *widget) {
    #ifdef NO_GTK_SUPPORT
    Q_UNUSED(widget)
    #else
    if (widget && wm) {
        wm->registerWidgetAndChildren(widget);
    }
    #endif
}
