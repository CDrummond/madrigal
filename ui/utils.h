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

#ifndef UI_UTILS_H
#define UI_UTILS_H

class QString;
class QWidget;
class QUrl;
class QRectF;

#include <QPainterPath>
#include <QColor>

namespace Ui {

namespace Utils {
    extern double smallFontFactor(const QFont &f);
    extern QFont smallFont(QFont f);
    extern int layoutSpacing(QWidget *w);
    extern double screenDpiScale();
    inline bool isHighDpi() { return screenDpiScale()>1.35; }
    inline int scaleForDpi(int v) { return qRound(screenDpiScale()*v); }
    extern bool limitedHeight(QWidget *w);
    extern void resizeWindow(QWidget *w, bool preserveWidth=true, bool preserveHeight=true);
    extern void raiseWindow(QWidget *w);

    enum Desktop {
        KDE,
        Gnome,
        Unity,
        Other
    };
    extern Desktop currentDe();
    extern QPainterPath buildPath(const QRectF &r, double radius);
    extern QColor clampColor(const QColor &col);
}

}

#endif
