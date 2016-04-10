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

#include "mac/notify.h"
#include "mac/notification.h"
#include "core/images.h"
#include "core/configuration.h"
#include "core/globalstatic.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include "notificationsinterface.h"
#include "config.h"
#include <QCoreApplication>
#include <QImage>

GLOBAL_STATIC(Mac::Notify, instance)

QDBusArgument& operator<< (QDBusArgument &arg, const QImage &image) {
    if (image.isNull()) {
        // Sometimes this gets called with a null QImage for no obvious reason.
        arg.beginStructure();
        arg << 0 << 0 << 0 << false << 0 << 0 << QByteArray();
        arg.endStructure();
        return arg;
    }
    QImage scaled = image.scaledToHeight(128, Qt::SmoothTransformation).convertToFormat(QImage::Format_ARGB32);

    #if Q_BYTE_ORDER == Q_LITTLE_ENDIAN
    // ABGR -> ARGB
    QImage i = scaled.rgbSwapped();
    #else
    // ABGR -> GBAR
    QImage i(scaled.size(), scaled.format());
    for (int y = 0; y < i.height(); ++y) {
        QRgb *p = (QRgb*) scaled.scanLine(y);
        QRgb *q = (QRgb*) i.scanLine(y);
        QRgb *end = p + scaled.width();
        while (p < end) {
            *q = qRgba(qGreen(*p), qBlue(*p), qAlpha(*p), qRed(*p));
            p++;
            q++;
        }
    }
    #endif

    arg.beginStructure();
    arg << i.width();
    arg << i.height();
    arg << i.bytesPerLine();
    arg << i.hasAlphaChannel();
    int channels = i.isGrayscale() ? 1 : (i.hasAlphaChannel() ? 4 : 3);
    arg << i.depth() / channels;
    arg << channels;
    arg << QByteArray(reinterpret_cast<const char*>(i.bits()), i.byteCount());
    arg.endStructure();
    return arg;
}

const QDBusArgument& operator>> (const QDBusArgument &arg, QImage &image) {
  // This is needed to link but shouldn't be called.
  Q_ASSERT(0);
  Q_UNUSED(image)
  return arg;
}

static const int constTimeout=5000;

Mac::Notify::Notify(QObject *p)
    : QObject(p)
    , lastId(0)
    , renderer(0)
{
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(setRenderer(QModelIndex)));
    enabled=Core::Configuration(this).get("enabled", true);
}

void Mac::Notify::setEnabled(bool en) {
    if (en!=enabled) {
        enabled=en;
        Core::Configuration(this).set("enabled", enabled);
    }
}

void Mac::Notify::setRenderer(Upnp::Renderer *r) {
    if (r==renderer) {
        return;
    }
    if (renderer) {
        disconnect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
    }
    renderer=r;
    if (renderer) {
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
        update(renderer->current());
    } else {
        update(QModelIndex());
    }
}

void Mac::Notify::setRenderer(const QModelIndex &idx) {
    setRenderer(static_cast<Upnp::Renderer *>(idx.internalPointer()));
}

void Mac::Notify::update(const QModelIndex &idx) {
    if (idx.isValid()) {
        update(*static_cast<Upnp::Device::MusicTrack *>(idx.internalPointer()));
    } else {
        update(Upnp::Device::MusicTrack());
    }
}

void Mac::Notify::update(const Upnp::Device::MusicTrack &song)
{
    if (song.artistName()!=currentSong.artistName() || song.album!=currentSong.album ||
        song.track!=currentSong.track || song.name!=currentSong.name) {
        currentSong = song;
        if (!currentSong.artist.isEmpty() && enabled) {
            QImage *i=Core::Images::self()->get(Core::ImageDetails(song.artUrl, song.artistName(), song.album), 0, true);
            showNotification(tr("Now playing"), song.describe(), i ? *i : QImage());
        }
    }
}
