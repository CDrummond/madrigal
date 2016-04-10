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

#include "ui/statuslabel.h"
#include <QTimer>
#include <QStyle>

Ui::StatusLabel::StatusLabel(QWidget *p)
    : Ui::SqueezedTextLabel(p)
    , timer(0)
{
    setVisible(false);
    QColor bg=QColor(0x98, 0xBC, 0xE3);
    QColor fg=palette().color(QPalette::WindowText);
    QColor border = Qt::blue;

    setStyleSheet(
        QString("QLabel {"
            "background-color: %1;"
            "border: 1px solid %2;"
            "margin: %3px;"
            "font-weight: bold;"
            "}"
            )
                .arg(bg.name())
                .arg(border.name())
                // DefaultFrameWidth returns the size of the external margin + border width. We know our border is 1px,
                // so we subtract this from the frame normal QStyle FrameWidth to get our margin
                .arg(style()->pixelMetric(QStyle::PM_DefaultFrameWidth, 0, this) -1));
    setContentsMargins(8, 8, 8, 8);
    setFixedHeight(fontMetrics().height()*2);
}

Ui::StatusLabel::~StatusLabel() {

}

void Ui::StatusLabel::setText(const QString &text, int time) {
    SqueezedTextLabel::setText(text);
    if (text.isEmpty()) {
        setVisible(false);
        if (timer) {
            timer->stop();
        }
    } else {
        setVisible(true);
        if (!timer) {
            timer=new QTimer(this);
            timer->setSingleShot(true);
            connect(timer, SIGNAL(timeout()), this, SLOT(timeout()));
        }
        timer->start(time*1000);
    }
}

void Ui::StatusLabel::timeout() {
    setVisible(false);
}
