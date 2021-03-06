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

#ifndef UI_TOOLBAR_H
#define UI_TOOLBAR_H

#include <QToolBar>
#include <QIcon>
#include "upnp/renderer.h"

class QModelIndex;
class QMenu;

namespace Ui {
class Action;
class NowPlayingWidget;
class VolumeSlider;

class ToolBar : public QToolBar {
    Q_OBJECT

public:
    ToolBar(QWidget *parent);
    virtual ~ToolBar() { }

    void init();
    void addMenuButton(QMenu *mnu);
    void setLyricsVisible(bool v);
    bool isLyricsVisible() const;

Q_SIGNALS:
    void showLyrics(bool s);

private Q_SLOTS:
    void setRenderer(const QModelIndex &idx);
    void playbackState(Upnp::Renderer::State state);
    void modelReset();
    void controlButtons();

private:
    void addSpacer();
    void enableControls(bool en);

private:
    Action *prevAction;
    Action *playPauseAction;
    Action *nextAction;
    Action *showLyricsAction;
    NowPlayingWidget *nowPlaying;
    VolumeSlider *volumeSlider;
    Upnp::Renderer *renderer;
    QIcon playIcon;
    QIcon pauseIcon;
};

}

#endif
