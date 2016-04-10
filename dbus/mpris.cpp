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

#include "dbus/mpris.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include "playeradaptor.h"
#include "rootadaptor.h"
#include "config.h"

static inline qlonglong convertTime(int t) {
    return t*1000000;
}

static QString mprisPath;

Dbus::Mpris::Mpris(QObject *p)
    : QObject(p)
    , renderer(0)
{
    QDBusConnection::sessionBus().registerService("org.mpris.MediaPlayer2." PACKAGE_NAME);

    new PlayerAdaptor(this);
    new MediaPlayer2Adaptor(this);

    QDBusConnection::sessionBus().registerObject("/org/mpris/MediaPlayer2", this, QDBusConnection::ExportAdaptors);

    if (mprisPath.isEmpty()) {
        mprisPath=QLatin1String(APP_REV_URL);
        mprisPath.replace(".", "/");
        mprisPath="/"+mprisPath+"/Track/%1";
    }
    connect(Core::Images::self(), SIGNAL(found(Core::ImageDetails)), this, SLOT(updateCurrentCover(Core::ImageDetails)));
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(setRenderer(QModelIndex)));
}

Dbus::Mpris::~Mpris() {
    QDBusConnection::sessionBus().unregisterService("org.mpris.MediaPlayer2." PACKAGE_NAME);
}

void Dbus::Mpris::setRenderer(Upnp::Renderer *r) {
    if (r==renderer) {
        return;
    }
    if (renderer) {
        disconnect(renderer, SIGNAL(playbackState(Upnp::Renderer::State)), this, SLOT(updateStatus()));
        disconnect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
    }
    renderer=r;
    if (renderer) {
        connect(renderer, SIGNAL(playbackState(Upnp::Renderer::State)), this, SLOT(updateStatus()));
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
        update(renderer->current());
    } else {
        update(QModelIndex());
    }
    updateStatus();
}

void Dbus::Mpris::setRenderer(const QModelIndex &idx) {
    setRenderer(static_cast<Upnp::Renderer *>(idx.internalPointer()));
}

void Dbus::Mpris::Next() {
    if (renderer) {
        renderer->next();
    }
}

void Dbus::Mpris::Previous() {
    if (renderer) {
        renderer->previous();
    }
}

void Dbus::Mpris::Pause() {
    if (renderer && Upnp::Renderer::Playing==renderer->playback().state) {
        renderer->playPause();
    }
}

void Dbus::Mpris::PlayPause() {
    if (renderer) {
        renderer->playPause();
    }
}

void Dbus::Mpris::Stop() {
    if (renderer) {
        renderer->stop();
    }
}

void Dbus::Mpris::Play() {
    if (renderer && Upnp::Renderer::Playing!=renderer->playback().state) {
        renderer->playPause();
    }
}

void Dbus::Mpris::Seek(qlonglong pos) {
    if (renderer) {
        renderer->seek(pos/1000000);
    }
}

void Dbus::Mpris::SetPosition(const QDBusObjectPath &, qlonglong pos) {
    if (renderer) {
        renderer->seek(pos/1000000);
    }
}

QString Dbus::Mpris::PlaybackStatus() {
    if (!renderer) {
        return QLatin1String("Stopped");
    }
    switch(renderer->playback().state) {
    case Upnp::Renderer::Playing: return QLatin1String("Playing");
    case Upnp::Renderer::Paused: return QLatin1String("Paused");
    default: return QLatin1String("Stopped");
    }
}

QString Dbus::Mpris::LoopStatus() {
    return renderer && renderer->playback().repeat ? QLatin1String("Playlist") : QLatin1String("None");
}

void Dbus::Mpris::SetLoopStatus(const QString &s) {
    if (renderer) {
        renderer->setRepeat(QLatin1String("None")!=s);
    }
}

qlonglong Dbus::Mpris::Position() const
{
    return convertTime(renderer ? renderer->playback().seconds : 0.0);
}

void Dbus::Mpris::updateStatus()
{
    QVariantMap map;

    map.insert("LoopStatus", LoopStatus());
    map.insert("Shuffle", Shuffle());
    map.insert("Volume", Volume());
    map.insert("CanGoNext", CanGoNext());
    map.insert("CanGoPrevious", CanGoPrevious());
    map.insert("PlaybackStatus", PlaybackStatus());
    map.insert("CanPlay", CanPlay());
    map.insert("CanPause", CanPause());
    map.insert("CanSeek", CanSeek());
    map.insert("Position", Position());
    map.insert("Metadata", Metadata());
    signalUpdate(map);
}

void Dbus::Mpris::updateCurrentCover(const Core::ImageDetails &image)
{
    if (image.artist==currentSong.artistName() && image.album==currentSong.album) {
        QImage *i=Core::Images::self()->get(Core::ImageDetails(currentSong.artUrl, currentSong.artistName(), currentSong.album), 0, true);
        if (i && i->text(Core::Images::constCacheFilename)!=currentCover) {
            currentCover=i->text(Core::Images::constCacheFilename);
            signalUpdate("Metadata", Metadata());
        }
    }
}

void Dbus::Mpris::update(const QModelIndex &idx) {
    if (idx.isValid()) {
        update(*static_cast<Upnp::Device::MusicTrack *>(idx.internalPointer()));
    } else {
        update(Upnp::Device::MusicTrack());
    }
}

void Dbus::Mpris::update(const Upnp::Device::MusicTrack &song)
{
    if (song.artistName()!=currentSong.artistName() || song.album!=currentSong.album ||
        song.track!=currentSong.track || song.name!=currentSong.name) {
        if (song.artistName()!=currentSong.artistName() || song.album!=currentSong.album) {
            QImage *i=Core::Images::self()->get(Core::ImageDetails(song.artUrl, song.artistName(), song.album), 0, true);
            if (i) {
                currentCover=i->text(Core::Images::constCacheFilename);
            }
        }
        currentSong = song;
        signalUpdate("Metadata", Metadata());
    }
}

QVariantMap Dbus::Mpris::Metadata() const {
    QVariantMap metadataMap;
    if (!currentSong.name.isEmpty() && !currentSong.artist.isEmpty()) {
//        metadataMap.insert("mpris:trackid", QVariant::fromValue<QDBusObjectPath>(QDBusObjectPath(mprisPath.arg(currentSong.id))));
        if (currentSong.duration>0) {
            metadataMap.insert("mpris:length", convertTime(currentSong.duration));
        }
        if (!currentSong.album.isEmpty()) {
            metadataMap.insert("xesam:album", currentSong.album);
        }
        if (!currentSong.albumArtist.isEmpty() && currentSong.albumArtist!=currentSong.artist) {
            metadataMap.insert("xesam:albumArtist", QStringList() << currentSong.albumArtist);
        }
        metadataMap.insert("xesam:artist", QStringList() << currentSong.artist);
        metadataMap.insert("xesam:title", currentSong.name);
        if (!currentSong.genre.isEmpty()) {
            metadataMap.insert("xesam:genre", QStringList() << currentSong.genre);
        }
        if (currentSong.track>0) {
            metadataMap.insert("xesam:trackNumber", currentSong.track);
        }
//        if (currentSong.disc>0) {
//            metadataMap.insert("xesam:discNumber", currentSong.disc);
//        }
        if (currentSong.year>0) {
            metadataMap.insert("xesam:contentCreated", QString("%04d").arg(currentSong.year));
        }
        if (!currentSong.url.isEmpty()) {
            metadataMap.insert("xesam:url", currentSong.url);
        }
        if (!currentCover.isEmpty()) {
             metadataMap.insert("mpris:artUrl", "file://"+currentCover);
        }
    }

    return metadataMap;
}

bool Dbus::Mpris::Shuffle() {
    return renderer && renderer->playback().shuffle;
}

void Dbus::Mpris::SetShuffle(bool s) {
    if (renderer) {
        renderer->setShuffle(s);
    }
}

double Dbus::Mpris::Volume() const {
    return renderer ? renderer->volume().current/renderer->volume().max : 0.0;
}

void Dbus::Mpris::SetVolume(double v) {
    if (renderer) {
        renderer->setVolume(renderer->volume().max*v);
    }
}

bool Dbus::Mpris::CanPlay() const {
    return renderer && Upnp::Renderer::Playing!=renderer->playback().state && !renderer->isEmpty();
}

bool Dbus::Mpris::CanPause() const {
    return renderer && Upnp::Renderer::Playing==renderer->playback().state;
}

bool Dbus::Mpris::CanSeek() const {
    return renderer && renderer->current().isValid();
}
bool Dbus::Mpris::CanGoNext() const {
    return renderer && Upnp::Renderer::Stopped!=renderer->playback().state && renderer->numItems()>1;
}

bool Dbus::Mpris::CanGoPrevious() const {
    return CanGoNext();
}

void Dbus::Mpris::Raise() {
    emit showMainWindow();
}

void Dbus::Mpris::signalUpdate(const QString &property, const QVariant &value) {
    QVariantMap map;
    map.insert(property, value);
    signalUpdate(map);
}

void Dbus::Mpris::signalUpdate(const QVariantMap &map) {
    if (map.isEmpty()) {
        return;
    }
    QDBusMessage signal = QDBusMessage::createSignal("/org/mpris/MediaPlayer2",
                                                     "org.freedesktop.DBus.Properties",
                                                     "PropertiesChanged");
    QVariantList args = QVariantList()
                          << "org.mpris.MediaPlayer2.Player"
                          << map
                          << QStringList();
    signal.setArguments(args);
    QDBusConnection::sessionBus().send(signal);
}
