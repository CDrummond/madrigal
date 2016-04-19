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

#include "ui/notification.h"
#include "ui/utils.h"
#include <QFontMetrics>
#include <QPainter>
#include <QTimer>
#include <QEvent>

Ui::Notification::Notification(QWidget *p)
    : QWidget(p)
    , offset(64)
    , timer(0)
    , opacityValue(0.0)
{
    spacing=fontMetrics().height();
    setVisible(false);
    setFixedHeight(2*spacing);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    anim.setPropertyName("opacity");
    anim.setTargetObject(this);
}

void Ui::Notification::show(const QString &msg, int timeout) {
    text=msg;
    if (!text.isEmpty()) {
        setVisible(true);
    }
    startAnimation(text.isEmpty() ? 0.0 : 1.0);
    if (!text.isEmpty()) {
        setSizeAndPosition();
        update();
        if (-1!=timeout) {
            if (!timer) {
                timer=new QTimer(this);
                connect(timer, SIGNAL(timeout()), this, SLOT(close()));
                timer->setSingleShot(true);
            }
            timer->start(timeout*1000);
        }
    }
}

void Ui::Notification::close() {
    if (timer) {
        timer->stop();
    }
    if (isVisible()) {
        startAnimation(0.0);
    } else {
        opacityValue=0;
    }
}

void Ui::Notification::paintEvent(QPaintEvent *) {
    QPainter p(this);
    QRect r(rect());
    QRectF rf(r.x()+0.5, r.y()+0.5, r.width()-1, r.height()-1);
    QColor col(Qt::black);
    col.setAlphaF(0.65);

    p.setOpacity(opacityValue);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillPath(Utils::buildPath(rf, rf.height()/4.0), col);
    p.setPen(Qt::white);
    rf.adjust(spacing, 0, -spacing, 0);
    p.drawText(rf, fontMetrics().elidedText(text, Qt::ElideRight, rf.width()), QTextOption(Qt::AlignCenter));
}


bool Ui::Notification::eventFilter(QObject *obj, QEvent *ev) {
    if (obj==parentWidget() && QEvent::Resize==ev->type() && isVisible()) {
        setSizeAndPosition();
    }
    return QWidget::eventFilter(obj, ev);
}

void Ui::Notification::setVisible(bool visible) {
    parentWidget()->removeEventFilter(this);
    if (visible) {
        parentWidget()->installEventFilter(this);
    }
    QWidget::setVisible(visible);
}

void Ui::Notification::setOpacity(float v) {
    if (!qFuzzyCompare(v, opacityValue)) {
        opacityValue=v;
        if (qFuzzyCompare(opacityValue, 0.0F)) {
            setVisible(false);
        } else {
            update();
        }
    }
}

void Ui::Notification::setSizeAndPosition() {
    int textWidth=fontMetrics().width(text)+(spacing*2)+4;
    int desiredWidth=qMin(textWidth, parentWidget()->width()-(spacing*2));
    if (width()!=desiredWidth) {
        resize(desiredWidth, height());
    }

    QPoint desiredPos((parentWidget()->width()-desiredWidth)/2,
                      offset>=0 ? offset : (parentWidget()->height()-offset));
    if (pos()!=desiredPos) {
        move(desiredPos);
    }
}

void Ui::Notification::startAnimation(float end) {
    if (!qFuzzyCompare(opacityValue, end)) {
        anim.stop();
        anim.setDuration(250);
        anim.setEndValue(end);
        anim.start();
    }
}
