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

#ifndef DBUS_MPRIS_H
#define DBUS_MPRIS_H

#include <QObject>
#include <QStringList>
#include <QVariantMap>
#include <QApplication>
#include "config.h"
#include "upnp/renderer.h"
#include "core/images.h"

class QDBusObjectPath;

namespace Dbus {
class Mpris : public QObject
{
    Q_OBJECT

    // org.mpris.MediaPlayer2.Player
    Q_PROPERTY( double Rate READ Rate WRITE SetRate )
    Q_PROPERTY( qlonglong Position READ Position )
    Q_PROPERTY( double MinimumRate READ MinimumRate )
    Q_PROPERTY( double MaximumRate READ MaximumRate )
    Q_PROPERTY( bool CanControl READ CanControl )
    Q_PROPERTY( bool CanPlay READ CanPlay )
    Q_PROPERTY( bool CanPause READ CanPause )
    Q_PROPERTY( bool CanSeek READ CanSeek )
    Q_PROPERTY( bool CanGoNext READ CanGoNext )
    Q_PROPERTY( bool CanGoPrevious READ CanGoPrevious )
    Q_PROPERTY( QString PlaybackStatus READ PlaybackStatus )
    Q_PROPERTY( QString LoopStatus READ LoopStatus WRITE SetLoopStatus )
    Q_PROPERTY( bool Shuffle READ Shuffle WRITE SetShuffle )
    Q_PROPERTY( QVariantMap Metadata READ Metadata )
    Q_PROPERTY( double Volume READ Volume WRITE SetVolume )

    // org.mpris.MediaPlayer2
    Q_PROPERTY( bool CanQuit READ CanQuit )
    Q_PROPERTY( bool CanRaise READ CanRaise )
    Q_PROPERTY( QString DesktopEntry READ DesktopEntry )
    Q_PROPERTY( bool HasTrackList READ HasTrackList )
    Q_PROPERTY( QString Identity READ Identity )
    Q_PROPERTY( QStringList SupportedMimeTypes READ SupportedMimeTypes )
    Q_PROPERTY( QStringList SupportedUriSchemes READ SupportedUriSchemes )

public:
    Mpris(QObject *p=0);
    virtual ~Mpris();

    // org.mpris.MediaPlayer2.Player
    void Next();
    void Previous();
    void Pause();
    void PlayPause();
    void Stop();
    void Play();
    void Seek(qlonglong pos);
    void SetPosition(const QDBusObjectPath &, qlonglong pos);
    void OpenUri(const QString &) { }
    QString PlaybackStatus();
    QString LoopStatus();
    void SetLoopStatus(const QString &s);
    QVariantMap Metadata() const;
    int Rate() const { return 1.0; }
    void SetRate(double) { }
    bool Shuffle();
    void SetShuffle(bool s);
    double Volume() const;
    void SetVolume(double v);
    qlonglong Position() const;
    double MinimumRate() const { return 1.0; }
    double MaximumRate() const { return 1.0; }
    bool CanControl() const { return true; }
    bool CanPlay() const;
    bool CanPause() const;
    bool CanSeek() const;
    bool CanGoNext() const;
    bool CanGoPrevious() const;

    // org.mpris.MediaPlayer2
    bool CanQuit() const { return true; }
    bool CanRaise() const { return true; }
    bool HasTrackList() const { return false; }
    QString Identity() const { return QLatin1String(PACKAGE_NAME_CASE); }
    QString DesktopEntry() const { return QLatin1String(PACKAGE_NAME); }
    QStringList SupportedUriSchemes() const { return QStringList(); }
    QStringList SupportedMimeTypes() const { return QStringList(); }

public:
    void update(const Upnp::Device::MusicTrack &song);

public Q_SLOTS:
    void Raise();
    void Quit() { QApplication::quit(); }

Q_SIGNALS:
    void showMainWindow();

public Q_SLOTS:
    void setRenderer(Upnp::Renderer *r);
    void setRenderer(const QModelIndex &idx);
    void updateCurrentCover(const Core::ImageDetails &image);

private Q_SLOTS:
    void update(const QModelIndex &idx);
    void updateStatus();

private:
    void signalUpdate(const QString &property, const QVariant &value);
    void signalUpdate(const QVariantMap &map);

private:
    Upnp::Renderer *renderer;
    Upnp::Device::MusicTrack currentSong;
    QString currentCover;
};

}

#endif
