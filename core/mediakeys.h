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

#ifndef CORE_MEDIA_KEYS_H
#define CORE_MEDIA_KEYS_H

#include <QObject>
#include "upnp/renderer.h"

namespace Core {

class MediaKeys : public QObject {
    Q_OBJECT
public:
    MediaKeys(QObject *p);
    virtual ~MediaKeys() { }

    virtual bool activate() = 0;
    virtual void deactivate() = 0;

private Q_SLOTS:
    void setRenderer(const QModelIndex &idx);

protected:
    void previous();
    void playPause();
    void next();
    void stop();

private:
    Upnp::Renderer *renderer;
};

}

#endif
