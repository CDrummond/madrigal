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

#include "ui/toolbutton.h"
#ifdef Q_OS_MAC
#include "ui/utils.h"
#endif
#include <QMenu>
#include <QStylePainter>
#include <QStyleOptionToolButton>
#include <QApplication>
#include <QPainter>

Ui::ToolButton::ToolButton(QWidget *parent)
    : QToolButton(parent)
    , hideMenuIndicator(true)
{
//    Icon::init(this);
    #ifdef Q_OS_MAC
    setStyleSheet("QToolButton {border: 0}");
    allowMouseOver=parent && parent->objectName()!=QLatin1String("toolbar");
    #endif
    setFocusPolicy(Qt::NoFocus);
    setAutoRaise(true);
}

void Ui::ToolButton::paintEvent(QPaintEvent *e) {
    #ifdef Q_OS_MAC
    bool down=isDown() || isChecked();
    bool mo=false;

    if (allowMouseOver && !down && isEnabled()) {
        QStyleOptionToolButton opt;
        initStyleOption(&opt);
        mo=opt.state&QStyle::State_MouseOver && this==QApplication::widgetAt(QCursor::pos());
    }
    if (down || mo) {
        QPainter p(this);
        QColor col(palette().color(QPalette::WindowText));
        QRect r(rect());
        int adjust=Utils::scaleForDpi(1);
        QPainterPath path=Utils::buildPath(QRectF(r.x()+0.5+adjust, r.y()+0.5+adjust, r.width()-((adjust*2)+1), r.height()-((adjust*2)+1)), 0.5+(adjust*2));
        p.setRenderHint(QPainter::Antialiasing, true);
        col.setAlphaF(0.4);
        p.setPen(col);
        p.drawPath(path);
        if (down) {
            col.setAlphaF(0.1);
            p.fillPath(path, col);
        }
    }
    #endif
    Q_UNUSED(e)
    // Hack to work-around Qt5 sometimes leaving toolbutton in 'raised' state.
    QStylePainter p(this);
    QStyleOptionToolButton opt;
    initStyleOption(&opt);
    if (hideMenuIndicator) {
        opt.features=QStyleOptionToolButton::None;
    }
    if (opt.state&QStyle::State_MouseOver && this!=QApplication::widgetAt(QCursor::pos())) {
        opt.state&=~QStyle::State_MouseOver;
    }
    #ifdef UNITY_MENU_HACK
    if (!icon.isNull()) {
        opt.icon=icon;
        opt.toolButtonStyle=Qt::ToolButtonIconOnly;
    }
    #endif
    p.drawComplexControl(QStyle::CC_ToolButton, opt);
}

void Ui::ToolButton::setMenu(QMenu *m) {
    QToolButton::setMenu(m);
    sh=QSize();
    setPopupMode(InstantPopup);
}
