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

#ifndef UI_NOTIFICATION_H
#define UI_NOTIFICATION_H

#include <QWidget>
#include <QPropertyAnimation>

class QTimer;

namespace Ui {

class Notification : public QWidget {
    Q_OBJECT
    Q_PROPERTY(float opacity READ opacity WRITE setOpacity)
public:
    Notification(QWidget *p);
    virtual ~Notification() { }
    void setOffset(int off) { offset=off; }

public Q_SLOTS:
    void show(const QString &msg, int timeout=-1);
    void close();

private:
    void paintEvent(QPaintEvent *);
    bool eventFilter(QObject *obj, QEvent *ev);
    void setVisible(bool visible);
    float opacity() const { return opacityValue; }
    void setOpacity(float v);
    void setSizeAndPosition();
    void startAnimation(float end);

private:
    int offset;
    int spacing;
    QString text;
    QTimer *timer;
    QPropertyAnimation anim;
    float opacityValue;
};
}
#endif
