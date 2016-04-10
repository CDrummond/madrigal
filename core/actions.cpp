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

#include "core/actions.h"
#include "core/monoicon.h"
#include <QMap>

static QMap<Core::Actions::Type, QIcon> icons;
static QColor iconColor(Qt::black);

void Core::Actions::setColor(const QColor &col) {
    iconColor=col;
}

QIcon Core::Actions::icon(Type t) {
    QMap<Type, QIcon>::ConstIterator it=icons.find(t);
    if (icons.end()!=it) {
        return it.value();
    }
    QIcon icn;

    switch (t) {
    case Action_Add: icn=Core::MonoIcon::icon(Core::MonoIcon::plus, iconColor, iconColor); break;
    case Action_Play: icn=Core::MonoIcon::icon(Core::MonoIcon::ex_mediaplay, iconColor, iconColor); break;
    default: break;
    }
    if (!icn.isNull()) {
        icons.insert(t, icn);
    }
    return icn;
}

QString Core::Actions::toolTip(Type t) {
    switch (t) {
    case Action_Add: return QObject::tr("Append to play queue");
    case Action_Play: return QObject::tr("Add and play");
    default: return QString();
    }
}
