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

#ifndef UI_TOOLBUTTON_H
#define UI_TOOLBUTTON_H

#include <QToolButton>
#include "config.h"

class QMenu;

namespace Ui {
class ToolButton : public QToolButton {
public:
    explicit ToolButton(QWidget *parent = 0);
    void setMenu(QMenu *m);
    void paintEvent(QPaintEvent *e);
    void setHideMenuIndicator(bool h) { hideMenuIndicator=h; }
    #ifdef UNITY_MENU_HACK
    void setIcon(const QIcon &i) { icon=i; }
    #endif

private:
    bool hideMenuIndicator;
    mutable QSize sh;
    #ifdef UNITY_MENU_HACK
    QIcon icon;
    #endif
    #ifdef Q_OS_MAC
    bool allowMouseOver;
    #endif
};
}

#endif
