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

#ifndef UI_NOWPLAYING_WIDGET_H
#define UI_NOWPLAYING_WIDGET_H

#include <QWidget>
#include <QSlider>
#include <QLabel>
#include "upnp/device.h"
#include "core/images.h"
#include "ui/squeezedtextlabel.h"

class QTimer;

namespace Ui {

class TimeLabel : public QLabel {
    Q_OBJECT
public:
    TimeLabel(QWidget *p, QSlider *s);
    virtual ~TimeLabel();
    void setRange(int min, int max);
    bool event(QEvent *e);

public Q_SLOTS:
    void updateTime();

protected:
    QSlider *slider;
    bool pressed;
    bool showRemaining;
};

class PosSlider : public QSlider {
    Q_OBJECT
public:
    PosSlider(QWidget *p);
    virtual ~PosSlider() { }

    void updateStyleSheet();
    void mouseMoveEvent(QMouseEvent *e);
    void wheelEvent(QWheelEvent *ev);
    void setRange(int min, int max);

Q_SIGNALS:
    void positionSet();
};

class NowPlayingWidget : public QWidget {
    Q_OBJECT
public:
    NowPlayingWidget(QWidget *p);
    virtual ~NowPlayingWidget() { }
    void clearTimes();
    int value() const;
    void setEnabled(bool e) { slider->setEnabled(e); }
    bool isEnabled() const { return slider->isEnabled(); }
    QColor textColor() const { return track->palette().windowText().color(); }

public Q_SLOTS:
    void update(const QModelIndex &idx);
    void updatePos(quint32 val);
    void updateDuration(quint32 val);

Q_SIGNALS:
    void seek(quint32 val);

private Q_SLOTS:
    void sliderReleased();
    void coverLoaded(const Core::ImageDetails &image);

private:
    void showEvent(QShowEvent *e);
    void updateCover(const Core::ImageDetails *cvr);

private:
    bool shown;
    QLabel *cover;
    SqueezedTextLabel *track;
    SqueezedTextLabel *artist;
    TimeLabel *time;
    PosSlider *slider;
    Core::ImageDetails currentCover;
};
}

#endif
