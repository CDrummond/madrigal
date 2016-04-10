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

#include "ui/coverwidget.h"

static const int constBorder=1;

Ui::CoverWidget::CoverWidget(QWidget *p)
    : QLabel(p)
    , pressed(false)
{
}

void Ui::CoverWidget::paintEvent(QPaintEvent *) {
    if (pix.isNull()) {
        return;
    }
    QPainter p(this);
    QSize layoutSize = pix.size() / pix.devicePixelRatio();
    QRect r((width()-layoutSize.width())/2, (height()-layoutSize.height())/2, layoutSize.width(), layoutSize.height());
    p.drawPixmap(r, pix);
    if (underMouse()) {
        #ifdef Q_OS_MAC
        QPen pen(OSXStyle::self()->viewPalette().color(QPalette::Highlight), 2);
        #else
        QPen pen(palette().color(QPalette::Highlight), 2);
        #endif
        pen.setJoinStyle(Qt::MiterJoin);
        p.setPen(pen);
        p.drawRect(r.adjusted(1, 1, -1, -1));
    }
}

void Ui::CoverWidget::updatePix() {
    QImage img=CurrentCover::self()->image();
    if (img.isNull()) {
        return;
    }
    int size=height();
    if (style()->pixelMetric(QStyle::PM_ToolBarFrameWidth)==0) {
        size-=constBorder*2;
    }
    double pixRatio=1.0;
    if (Settings::self()->retinaSupport()) {
        pixRatio=qApp->devicePixelRatio();
        size*=pixRatio;
    }
    img=img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    img.setDevicePixelRatio(pixRatio);
    if (pix.isNull() || pix.size()!=img.size()) {
        pix=QPixmap(img.size());
        pix.setDevicePixelRatio(pixRatio);
    }
    pix.fill(Qt::transparent);
    QPainter painter(&pix);
    painter.drawImage(0, 0, img);
    repaint();
}

void Ui::CoverWidget::deletePix() {
    if (!pix.isNull()) {
        pix=QPixmap();
    }
}
