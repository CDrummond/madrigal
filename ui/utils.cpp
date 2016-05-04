/*
 * Cantata
 *
 * Copyright (c) 2011-2016 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "ui/utils.h"
#include "core/utils.h"
#include "config.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QEventLoop>
#include <QStyle>
#include <QProcess>

double Ui::Utils::smallFontFactor(const QFont &f)
{
    double sz=f.pointSizeF();
    if (sz<=8.5) {
        return 1.0;
    }
    if (sz<=9.0) {
        return 0.9;
    }
    return 0.85;
}

QFont Ui::Utils::smallFont(QFont f)
{
    f.setPointSizeF(f.pointSizeF()*smallFontFactor(f));
    return f;
}

int Ui::Utils::layoutSpacing(QWidget *w)
{
    int spacing=(w ? w->style() : qApp->style())->layoutSpacing(QSizePolicy::DefaultType, QSizePolicy::DefaultType, Qt::Vertical);
    if (spacing<0) {
        spacing=scaleForDpi(4);
    }
    return spacing;
}

double Ui::Utils::screenDpiScale()
{
    static double scaleFactor=-1.0;
    if (scaleFactor<0) {
        QWidget *dw=QApplication::desktop();
        if (!dw) {
            return 1.0;
        }
        scaleFactor=dw->logicalDpiX()>120 ? qMin(qMax(dw->logicalDpiX()/96.0, 1.0), 4.0) : 1.0;
    }
    return scaleFactor;
}

bool Ui::Utils::limitedHeight(QWidget *w)
{
    static bool init=false;
    static bool limited=false;
    if (!init) {
         QDesktopWidget *dw=QApplication::desktop();
         if (dw) {
             limited=dw->availableGeometry(w).size().height()<=800;
         }
    }
    return limited;
}

void Ui::Utils::resizeWindow(QWidget *w, bool preserveWidth, bool preserveHeight)
{
    QWidget *window=w ? w->window() : 0;
    if (window) {
        QSize was=window->size();
        QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
        window->setMinimumSize(QSize(0, 0));
        window->adjustSize();
        QSize now=window->size();
        window->setMinimumSize(now);
        if (preserveWidth && preserveHeight) {
            window->resize(qMax(was.width(), now.width()), qMax(was.height(), now.height()));
        } else if (preserveWidth) {
            window->resize(qMax(was.width(), now.width()), now.height());
        } else if (preserveHeight) {
            window->resize(now.width(), qMax(was.height(), now.height()));
        }
    }
}

Ui::Utils::Desktop Ui::Utils::currentDe()
{
    #if !defined Q_OS_WIN && !defined Q_OS_MAC
    static int de=-1;
    if (-1==de) {
        de=Other;
        QByteArray desktop=qgetenv("XDG_CURRENT_DESKTOP").toLower();
        if ("unity"==desktop) {
            de=Unity;
        } else if ("kde"==desktop) {
            de=KDE;
        } else if ("gnome"==desktop || "pantheon"==desktop) {
            de=Gnome;
        } else {
            QByteArray kde=qgetenv("KDE_FULL_SESSION");
            if ("true"==kde) {
                de=KDE;
            }
        }
    }
    return (Utils::Desktop)de;
    #endif
    return Other;
}

QPainterPath Ui::Utils::buildPath(const QRectF &r, double radius)
{
    QPainterPath path;
    double diameter(radius*2);

    path.moveTo(r.x()+r.width(), r.y()+r.height()-radius);
    path.arcTo(r.x()+r.width()-diameter, r.y(), diameter, diameter, 0, 90);
    path.arcTo(r.x(), r.y(), diameter, diameter, 90, 90);
    path.arcTo(r.x(), r.y()+r.height()-diameter, diameter, diameter, 180, 90);
    path.arcTo(r.x()+r.width()-diameter, r.y()+r.height()-diameter, diameter, diameter, 270, 90);
    return path;
}

#ifdef Q_OS_WIN
// This is down here, because windows.h includes ALL windows stuff - and we get conflicts with MessageBox :-(
#include <windows.h>
#endif

void Ui::Utils::raiseWindow(QWidget *w)
{
    if (!w) {
        return;
    }

    #ifdef Q_OS_WIN
    ::SetWindowPos(reinterpret_cast<HWND>(w->effectiveWinId()), HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    ::SetWindowPos(reinterpret_cast<HWND>(w->effectiveWinId()), HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_SHOWWINDOW);
    #else
    w->showNormal();
    w->activateWindow();
    QApplication::processEvents(QEventLoop::ExcludeUserInputEvents);
    w->raise();
    w->activateWindow();
    #endif
}

QColor Ui::Utils::clampColor(const QColor &col) {
    static const int constMin=64;
    static const int constMax=240;

    if (col.value()<constMin) {
        return QColor(constMin, constMin, constMin);
    } else if (col.value()>constMax) {
        return QColor(constMax, constMax, constMax);
    }
    return col;
}
