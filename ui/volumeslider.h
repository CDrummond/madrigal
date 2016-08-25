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

#ifndef UI_VOLUMESLIDER_H
#define UI_VOLUMESLIDER_H

#include <QSlider>
#include <QColor>
#include "upnp/renderer.h"

class QPixmap;
class QMenu;
class QAction;

namespace Ui {
class Action;

class VolumeSlider : public QSlider {
    Q_OBJECT
public:
    static QColor clampColor(const QColor &col);

    VolumeSlider(QWidget *p=0);
    virtual ~VolumeSlider() { }

    void setColor(QColor col);
    void paintEvent(QPaintEvent *ev);
    void mousePressEvent(QMouseEvent *ev);
    void mouseReleaseEvent(QMouseEvent *ev);
    void contextMenuEvent(QContextMenuEvent *ev);
    void wheelEvent(QWheelEvent *ev);

Q_SIGNALS:
    void mute(bool m);
    void setVolume(int val);

public Q_SLOTS:
    void set(const Upnp::Renderer::Volume &vol);

private Q_SLOTS:
    void increaseVolume();
    void decreaseVolume();
    void changeVolume();
    void muteTriggered();

private:
    void generatePixmaps();
    QPixmap generatePixmap(bool filled);

private:
    int lineWidth;
    bool down;
    bool isMuted;
    QColor textCol;
    QPixmap pixmaps[2];
    Action *muteAction;
    QAction *muteMenuAction;
    Action *increaseAction;
    Action *decreaseAction;
    QMenu *menu;
};

}

#endif
