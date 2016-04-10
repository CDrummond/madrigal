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

#include "core/mediakeys.h"
#include "upnp/model.h"
#include "upnp/renderers.h"

Core::MediaKeys::MediaKeys(QObject *p)
    : QObject(p)
    , renderer(0)
{
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(setRenderer(QModelIndex)));
}

void Core::MediaKeys::setRenderer(const QModelIndex &idx) {
    renderer=static_cast<Upnp::Renderer *>(idx.internalPointer());
}

void Core::MediaKeys::previous() {
    if (renderer) {
        renderer->previous();
    }
}

void Core::MediaKeys::playPause() {
    if (renderer) {
        renderer->playPause();
    }
}

void Core::MediaKeys::next() {
    if (renderer) {
        renderer->next();
    }
}

void Core::MediaKeys::stop() {
    if (renderer) {
        renderer->stop();
    }
}
