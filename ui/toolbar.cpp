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

#include "ui/toolbar.h"
#include "ui/toolbutton.h"
#include "ui/gtkstyle.h"
#include "ui/nowplayingwidget.h"
#include "ui/volumeslider.h"
#include "ui/actioncollection.h"
#include "ui/action.h"
#include "ui/utils.h"
#include "ui/menubutton.h"
#include "core/monoicon.h"
#include "upnp/renderer.h"
#include "upnp/renderers.h"
#include "upnp/model.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>

Ui::ToolBar::ToolBar(QWidget *parent)
    : QToolBar(parent)
    , renderer(0)
{
    setObjectName("MainToolBar");
    setMovable(false);
    setContextMenuPolicy(Qt::NoContextMenu);
    GtkStyle::applyTheme(this);
    ensurePolished();
    QLabel lbl(this);
    lbl.ensurePolished();
    QColor col=Utils::clampColor(lbl.palette().text().color());

    playIcon=Core::MonoIcon::icon(Core::MonoIcon::ex_mediaplay, col, col);
    pauseIcon=Core::MonoIcon::icon(Core::MonoIcon::ex_mediapause, col, col);
    prevAction=ActionCollection::get()->createAction("previous", tr("Previous Track"), Core::MonoIcon::icon(Core::MonoIcon::ex_mediaprevious, col, col));
    playPauseAction=ActionCollection::get()->createAction("play", tr("Play/Pause"), playIcon);
    nextAction=ActionCollection::get()->createAction("next", tr("Next Track"), Core::MonoIcon::icon(Core::MonoIcon::ex_medianext, col, col));
    showLyricsAction=ActionCollection::get()->createAction("lyrics", tr("Show Lyrics"), Core::MonoIcon::icon(Core::MonoIcon::filetexto, col, col));
    showLyricsAction->setCheckable(true);

    playPauseAction->setShortcut(Qt::ControlModifier+Qt::Key_P);
    prevAction->setShortcut(Qt::ControlModifier+Qt::Key_Left);
    nextAction->setShortcut(Qt::ControlModifier+Qt::Key_Right);

    nowPlaying=new NowPlayingWidget(this);
    volumeSlider=new VolumeSlider(this);

    addSpacer();
    QWidget *controls=new QWidget(this);
    QHBoxLayout *controlsLayout=new QHBoxLayout(controls);
    QList<Action *> controlActs=QList<Action *>() << prevAction << playPauseAction << nextAction;
    int otherIconSize=Utils::scaleForDpi(28);
    int playPauseIconSize=Utils::scaleForDpi(32);
    int pad=Utils::scaleForDpi(8);
    foreach (Action *act, controlActs) {
        int iconSize=act==playPauseAction ? playPauseIconSize : otherIconSize;
        ToolButton *btn=new ToolButton(controls);
        btn->setDefaultAction(act);
        btn->setFixedSize(QSize(iconSize+pad, iconSize+pad));
        btn->setIconSize(QSize(iconSize, iconSize));
        btn->setAutoRaise(true);
        btn->setToolButtonStyle(Qt::ToolButtonIconOnly);
        controlsLayout->addWidget(btn);
    }
    controlsLayout->setMargin(0);

    int tbIconSize=Utils::scaleForDpi(22);
    setIconSize(QSize(tbIconSize, tbIconSize));

    addWidget(controls);
    addSpacer();
    addWidget(nowPlaying);
    addSpacer();
    addWidget(volumeSlider);
    addSpacer();
    ToolButton *lyricsButton=new ToolButton(this);
    lyricsButton->setDefaultAction(showLyricsAction);
    lyricsButton->setFixedSize(QSize(pad+tbIconSize, pad+tbIconSize));
    addWidget(lyricsButton);
    addSpacer();

    layout()->setSpacing(0);
    layout()->setMargin(0);
    nowPlaying->update(QModelIndex());
    enableControls(false);
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), SLOT(setRenderer(QModelIndex)));
    connect(showLyricsAction, SIGNAL(triggered(bool)), this, SIGNAL(showLyrics(bool)));
}

void Ui::ToolBar::addMenuButton(QMenu *mnu) {
    int btnSize=Utils::scaleForDpi(8)+iconSize().height();
    QLabel lbl(this);
    lbl.ensurePolished();
    QColor col=Utils::clampColor(lbl.palette().text().color());
    MenuButton *btn=new MenuButton(this);
    btn->setAlignedMenu(mnu);
    btn->setIcon(Core::MonoIcon::icon(Core::MonoIcon::bars, col, col));
    btn->setFixedSize(QSize(btnSize, btnSize));
    addWidget(btn);
}

void Ui::ToolBar::setLyricsVisible(bool v) {
    showLyricsAction->setChecked(v);
}

bool Ui::ToolBar::isLyricsVisible() const {
    return showLyricsAction->isChecked();
}

void Ui::ToolBar::setRenderer(const QModelIndex &idx) {
    Upnp::Renderer *r=idx.isValid() ? static_cast<Upnp::Renderer *>(idx.internalPointer()) : 0;
    if (r==renderer) {
        return;
    }
    if (renderer) {
        disconnect(prevAction, SIGNAL(triggered(bool)), renderer, SLOT(previous()));
        disconnect(playPauseAction, SIGNAL(triggered(bool)), renderer, SLOT(playPause()));
        disconnect(nextAction, SIGNAL(triggered(bool)), renderer, SLOT(next()));
        disconnect(volumeSlider, SIGNAL(mute(bool)), renderer, SLOT(mute(bool)));
        disconnect(volumeSlider, SIGNAL(setVolume(int)), renderer, SLOT(setVolume(int)));
        disconnect(nowPlaying, SIGNAL(seek(quint32)), renderer, SLOT(seek(quint32)));
        disconnect(renderer, SIGNAL(currentTrack(QModelIndex)), nowPlaying, SLOT(update(QModelIndex)));
        disconnect(renderer, SIGNAL(volumeState(const Upnp::Renderer::Volume&)), volumeSlider, SLOT(set(const Upnp::Renderer::Volume&)));
        disconnect(renderer, SIGNAL(playbackDuration(quint32)), nowPlaying, SLOT(updateDuration(quint32)));
        disconnect(renderer, SIGNAL(playbackPos(quint32)), nowPlaying, SLOT(updatePos(quint32)));
        disconnect(renderer, SIGNAL(playbackState(Upnp::Renderer::State)), this, SLOT(playbackState(Upnp::Renderer::State)));
        disconnect(renderer, SIGNAL(modelReset()), this, SLOT(modelReset()));
    }
    renderer=r;
    enableControls(0!=renderer);
    if (renderer) {
        connect(prevAction, SIGNAL(triggered(bool)), renderer, SLOT(previous()));
        connect(playPauseAction, SIGNAL(triggered(bool)), renderer, SLOT(playPause()));
        connect(nextAction, SIGNAL(triggered(bool)), renderer, SLOT(next()));
        connect(volumeSlider, SIGNAL(mute(bool)), renderer, SLOT(mute(bool)));
        connect(volumeSlider, SIGNAL(setVolume(int)), renderer, SLOT(setVolume(int)));
        connect(nowPlaying, SIGNAL(seek(quint32)), renderer, SLOT(seek(quint32)));
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), nowPlaying, SLOT(update(QModelIndex)));
        connect(renderer, SIGNAL(volumeState(const Upnp::Renderer::Volume&)), volumeSlider, SLOT(set(const Upnp::Renderer::Volume&)));
        connect(renderer, SIGNAL(playbackDuration(quint32)), nowPlaying, SLOT(updateDuration(quint32)));
        connect(renderer, SIGNAL(playbackPos(quint32)), nowPlaying, SLOT(updatePos(quint32)));
        connect(renderer, SIGNAL(playbackState(Upnp::Renderer::State)), this, SLOT(playbackState(Upnp::Renderer::State)));
        connect(renderer, SIGNAL(modelReset()), this, SLOT(modelReset()));
        nowPlaying->update(renderer->current());
        nowPlaying->updatePos(renderer->playback().seconds);
        nowPlaying->updateDuration(renderer->playback().duration);
        volumeSlider->set(renderer->volume());
    } else {
        nowPlaying->update(QModelIndex());
        nowPlaying->updatePos(0);
        nowPlaying->updateDuration(0);
        playbackState(Upnp::Renderer::Null);
        playPauseAction->setIcon(playIcon);
    }
}

void Ui::ToolBar::playbackState(Upnp::Renderer::State state) {
    int tracks=renderer ? renderer->rowCount() : 0;
    switch (state) {
    case Upnp::Renderer::Null:
        prevAction->setEnabled(false);
        playPauseAction->setEnabled(false);
        playPauseAction->setIcon(playIcon);
        nextAction->setEnabled(false);
        break;
    case Upnp::Renderer::Stopped:
        prevAction->setEnabled(tracks>1);
        playPauseAction->setEnabled(tracks>0);
        playPauseAction->setIcon(playIcon);
        nextAction->setEnabled(tracks>1);
        break;
    case Upnp::Renderer::Playing:
        prevAction->setEnabled(tracks>1);
        playPauseAction->setEnabled(true);
        playPauseAction->setIcon(pauseIcon);
        nextAction->setEnabled(tracks>1);
        break;
    case Upnp::Renderer::Paused:
        prevAction->setEnabled(tracks>1);
        playPauseAction->setEnabled(true);
        playPauseAction->setIcon(playIcon);
        nextAction->setEnabled(tracks>1);
        break;
    }
}

void Ui::ToolBar::modelReset() {
    playbackState(renderer && renderer->rowCount()>0 ? Upnp::Renderer::Stopped : Upnp::Renderer::Null);
}

void Ui::ToolBar::addSpacer() {
    QWidget *sep=new QWidget(this);
    sep->setFixedWidth(qMax(Utils::scaleForDpi(8), Utils::layoutSpacing(this)));
    addWidget(sep);
}

void Ui::ToolBar::enableControls(bool en) {
    volumeSlider->setEnabled(en);
    prevAction->setEnabled(en);
    playPauseAction->setEnabled(en);
    nextAction->setEnabled(en);
    nowPlaying->setEnabled(en);
}
