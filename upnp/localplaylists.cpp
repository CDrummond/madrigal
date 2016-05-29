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

#include "upnp/localplaylists.h"
#include "core/debug.h"
#include "core/utils.h"
#include "core/globalstatic.h"
#include <QDir>
#include <QFile>
#include <QXmlStreamReader>

GLOBAL_STATIC(Upnp::LocalPlaylists, instance)

static const char *constExt=".pl";
static const char *constDir="playlists";

static bool compareStrings(const QString &a, const QString &b) {
    int c=QString::localeAwareCompare(a.toUpper(), b.toUpper());

    if (0==c) {
        c=QString::localeAwareCompare(a, b);
    }
    return c<0;
}

Upnp::LocalPlaylists::LocalPlaylists()
    : MediaServer(Ssdp::Device("local:playlists", tr("Local Playlists")), 0)
{
}

Upnp::LocalPlaylists::~LocalPlaylists() {
}

void Upnp::LocalPlaylists::populate() {
    if (items.isEmpty()) {
        populate(QModelIndex());
    }
}

void Upnp::LocalPlaylists::populate(const QModelIndex &index, int start) {
    Q_UNUSED(start)
    QString dir=Core::Utils::dataDir(constDir, false);

    if (index.isValid()) {
        if (static_cast<Item *>(index.internalPointer())->isCollection()) {
            LocalPlaylist *pl=static_cast<LocalPlaylist *>(index.internalPointer());
            QFile f(dir+index.data().toString()+constExt);
            if (f.open(QIODevice::ReadOnly)) {
                QXmlStreamReader reader(&f);
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (reader.isStartElement() && QLatin1String("Track")==reader.name()) {
                        QXmlStreamReader trackReader(reader.readElementText());
                        while (!trackReader.atEnd()) {
                            trackReader.readNext();
                            if (trackReader.isStartElement() && QLatin1String("item")==trackReader.name()) {
                                beginInsertRows(index, pl->children.count(), pl->children.count());
                                pl->children.append(new MusicTrack(objectValues(trackReader), pl, pl->children.count()));
                                endInsertRows();
                                break;
                            }
                        }
                    }
                }
            }
            pl->state=State_Populated;
            checkCommand(index);
        }
    } else {
        if (!dir.isEmpty() && QDir(dir).exists()) {
            QStringList files=QDir(dir).entryList(QStringList() << QLatin1String("*")+constExt, QDir::Files);
            if (!files.isEmpty()) {
                QStringList names;
                foreach (const QString &file, files) {
                    names.append(file.left(file.length()-3));
                }
                qSort(names.begin(), names.end(), compareStrings);
                beginInsertRows(QModelIndex(), 0, names.count()-1);
                foreach (const QString &name, names) {
                    items.append(new LocalPlaylist(name, 0, items.count()));
                }
                endInsertRows();
            }
        }
    }
}

void Upnp::LocalPlaylists::save(const QString &name, const QByteArray &xml) {
    if (items.isEmpty()) {
        populate(QModelIndex());
    }
    QFile f(Core::Utils::dataDir(constDir, true)+name+constExt);
    if (f.open(QIODevice::WriteOnly|QIODevice::Text)) {
        f.write(xml);
        int r=0;
        for (; r<items.count(); ++r) {
            if (!compareStrings(items.at(r)->name, name)) {
                break;
            }
        }
        beginInsertRows(QModelIndex(), r, r);
        if (r<items.count()) {
            items.insert(r, new LocalPlaylist(name, 0, r));
            for (r=r+1; r<items.count(); ++r) {
                items.at(r)->row=r;
            }
        } else {
            items.append(new LocalPlaylist(name, 0, items.count()));
        }
        endInsertRows();
    }
}

bool Upnp::LocalPlaylists::exists(const QString &name) {
    if (items.isEmpty()) {
        populate(QModelIndex());
    }
    foreach (Item *i, items) {
        if (i->name==name) {
            return true;
        }
    }
    return false;
}

void Upnp::LocalPlaylists::remove(const QModelIndexList &indexes) {
    QString dir=Core::Utils::dataDir(constDir, false);

    foreach (const QModelIndex &index, indexes) {
        if (static_cast<Item *>(index.internalPointer())->isCollection()) {
            LocalPlaylist *pl=static_cast<LocalPlaylist *>(index.internalPointer());
            if (QFile::remove(dir+index.data().toString()+constExt)) {
                beginRemoveRows(QModelIndex(), pl->row, pl->row);
                items.removeAll(pl);
                delete pl;
                for (int i=pl->row; i<items.count(); ++i) {
                    items.at(i)->row=i;
                }
                endRemoveRows();
            }
        }
    }
}
