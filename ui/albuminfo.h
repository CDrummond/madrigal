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

#ifndef UI_ALBUM_INFO_H
#define UI_ALBUM_INFO_H

#include <QWidget>
#include "upnp/mediaserver.h"
#include "core/images.h"

class QLabel;

namespace Ui {
class SqueezedTextLabel;
class ToolButton;

class AlbumInfo : public QWidget {
    Q_OBJECT

public:
    AlbumInfo(QWidget *p);
    virtual ~AlbumInfo() { }

    void update(const Upnp::MediaServer::Album *info);
    bool event(QEvent *ev);

Q_SIGNALS:
    void play();
    void add();
    void clicked();

private:
    void updateCover(const Upnp::MediaServer::Album *info);

private Q_SLOTS:
    void coverLoaded(const Core::ImageDetails &image);

private:
    QString albumName;
    SqueezedTextLabel *name;
    SqueezedTextLabel *artist;
    SqueezedTextLabel *details;
    QLabel *cover;
    ToolButton *playButton;
    ToolButton *addButton;
    bool btnDown;
};
}
#endif
