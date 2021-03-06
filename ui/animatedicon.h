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


#ifndef UI_ANIMATED_ICON_H
#define UI_ANIMATED_ICON_H

#include <QWidget>
#include "core/monoicon.h"

class QTimer;

namespace Ui {
class AnimatedIcon : public QWidget {
    Q_OBJECT

public:
    AnimatedIcon(QWidget *parent);
    ~AnimatedIcon();
    void setIcon(Core::MonoIcon::Type icn);
    void setIconSize(int size);
    void start();
    void stop();
    void animate(bool on) { on ? start() : stop(); }
    void paintEvent(QPaintEvent *ev);

private Q_SLOTS:
    void rotate();

private:
    void create();

private:
    Core::MonoIcon::Type type;
    QIcon icon;
    QTimer *timer;
    QPixmap pix;
    int iconSize;
    int count;
};

}

#endif
