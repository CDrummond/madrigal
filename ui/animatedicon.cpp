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


#include "ui/animatedicon.h"
#include "ui/utils.h"
#include <QLabel>
#include <QTimer>
#include <QPixmap>
#include <QMatrix>
#include <QPainter>

static const int constNumSteps=16;

Ui::AnimatedIcon::AnimatedIcon(QWidget *parent)
    : QWidget(parent)
    , type(Core::MonoIcon::no_icon)
    , timer(0)
    , iconSize(0)
    , count(0)
{
}

Ui::AnimatedIcon::~AnimatedIcon() {
}

void Ui::AnimatedIcon::setIcon(Core::MonoIcon::Type icn) {
    if (icn!=type) {
        pix=QPixmap();
        type=icn;
    }
}

void Ui::AnimatedIcon::setIconSize(int size) {
    if (size!=iconSize) {
        pix=QPixmap();
        iconSize=size;
        int pad=Utils::scaleForDpi(4);
        setFixedSize(QSize(size+pad, size+pad));
    }
}

void Ui::AnimatedIcon::start() {
    count=0;
    create();
    if (!timer) {
        timer=new QTimer(this);
        connect(timer, SIGNAL(timeout()), SLOT(rotate()));
    }
    timer->start(1500/constNumSteps);
    update();
}

void Ui::AnimatedIcon::stop() {
    if (timer) {
        timer->stop();
    }
    count=0;
    update();
}

void Ui::AnimatedIcon::paintEvent(QPaintEvent *ev) {
    QWidget::paintEvent(ev);
    if (!pix.isNull()) {
        QPainter p(this);
        float centerX = width() * 0.5;
        float centerY = height() * 0.5;
        p.translate(centerX, centerY);
        p.rotate(count*(360.0/constNumSteps));
        p.translate(-centerX, -centerY);

        p.drawPixmap((width()-iconSize)/2, (height()-iconSize)/2, pix);
    }
}

void Ui::AnimatedIcon::rotate() {
    if (++count==constNumSteps) {
        count=0;
    }
    create();
    update();
}

void Ui::AnimatedIcon::create() {
    if (!pix.isNull() || iconSize<Utils::scaleForDpi(4)) {
        return;
    }
    QColor col=Utils::clampColor(palette().color(QPalette::WindowText));
    pix=Core::MonoIcon::icon(type, col, col).pixmap(iconSize, iconSize);
}

