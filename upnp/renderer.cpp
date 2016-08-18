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

#include "upnp/renderer.h"
#include "core/roles.h"
#include "config.h"
#include <QMimeData>
#include <QTimer>
#include <QXmlStreamWriter>

static const char * constRowListMimeType=APP_REV_URL"/row-list";

QString Upnp::Renderer::Track::subText() const {
    QString ar=artistName();
    return ar.isEmpty()
                ? album.isEmpty()
                    ? QString()
                    : album
                : album.isEmpty()
                    ? ar
                    : (ar+QLatin1String(" - ")+album);
}

QVariant Upnp::Renderer::data(const QModelIndex &index, int role) const {
    const Item *item = toItem(index);
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case Core::Role_IsCurrent:
        return static_cast<const Track *>(item)->id==currentTrackId;
    case Core::Role_PlayState:
        return static_cast<const Track *>(item)->id==currentTrackId ? playState.state : Null;
    default:
        return Device::data(index, role);
    }
}

Qt::ItemFlags Upnp::Renderer::flags(const QModelIndex &index) const {
    if (index.isValid()) {
        return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
    } else {
        return Qt::ItemIsDropEnabled;
    }
}

Qt::DropActions Upnp::Renderer::supportedDropActions() const {
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList Upnp::Renderer::mimeTypes() const {
    return QStringList() << constRowListMimeType << constObjectIdListMimeType;
}

QMimeData * Upnp::Renderer::mimeData(const QModelIndexList &indexes) const {
    QMimeData *mimeData = new QMimeData();
    QByteArray data;
    QDataStream stream(&data, QIODevice::WriteOnly);

    foreach(QModelIndex index, indexes) {
        if (index.isValid() && 0==index.column()) {
            stream << (quint32)index.row();
        }
    }

    mimeData->setData(constRowListMimeType, data);
    return mimeData;
}

bool Upnp::Renderer::dropMimeData(const QMimeData *data,
                                  Qt::DropAction action, int row, int /*column*/, const QModelIndex & /*parent*/) {
    if (Qt::IgnoreAction==action) {
        return true;
    }

    if (data->hasFormat(constRowListMimeType)) { //Act on internal moves
        QByteArray encodedData=data->data(constRowListMimeType);
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QList<quint32> rows;
        while (!stream.atEnd()) {
            quint32 v;
            stream >> v;
            rows.append(v);
        }

        moveRows(rows, row);
        return true;
    } else if (data->hasFormat(constObjectIdListMimeType)) {
        QByteArray encodedData=data->data(constObjectIdListMimeType);
        QDataStream stream(&encodedData, QIODevice::ReadOnly);
        QByteArray src;
        QList<QByteArray> ids;
        stream >> src;
        while (!stream.atEnd()) {
            QByteArray id;
            stream >> id;
            ids.append(id);
        }
        emit acceptDrop(src, ids, row);
        return true;
    }
    return false;
}

QByteArray Upnp::Renderer::toXml() const {
    QByteArray xml;
    QXmlStreamWriter writer(&xml);
    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String("Tracks"));
    foreach (const Item *item, items) {
        writer.writeStartElement(QLatin1String("Track"));
        writer.writeCharacters(static_cast<const MusicTrack *>(item)->toXml());
        writer.writeEndElement();
    }
    writer.writeEndElement();
    writer.writeEndDocument();
    return xml;
}

void Upnp::Renderer::updateStats() {
    quint32 duration=0;
    foreach (Item *i, items) {
        duration+=static_cast<const Track *>(i)->duration;
    }
    emit queueDetails(items.count(), duration);
    if (items.isEmpty()) {
        emit currentTrack(QModelIndex());
    }
}
