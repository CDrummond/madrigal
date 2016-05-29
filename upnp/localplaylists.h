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

#ifndef UPNP_LOCAL_PLAYLISTS_H
#define UPNP_LOCAL_PLAYLISTS_H

#include "upnp/mediaserver.h"

namespace Upnp {

class LocalPlaylists : public MediaServer {
    Q_OBJECT
public:

public:
    static LocalPlaylists * self();

    struct LocalPlaylist : public Playlist {
        LocalPlaylist(const QString &n=QString(), Item *p=0, int r=0)
            : Playlist(n, QByteArray(), p, r) { }
        virtual ~LocalPlaylist() { }
        virtual QVariant actions() const {
            QVariant v;
            v.setValue< QList<int> >(QList<int>() << Core::Actions::Action_Play << Core::Actions::Action_Add << Core::Actions::Action_Delete);
            return v;
        }
    };

    LocalPlaylists();
    virtual ~LocalPlaylists();

    virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::listalt; }
    virtual void populate();
    virtual void populate(const QModelIndex &index, int start=0);
    void save(const QString &name, const QByteArray &xml);
    bool exists(const QString &name);
    virtual void remove(const QModelIndexList &indexes);
};

}

#endif
