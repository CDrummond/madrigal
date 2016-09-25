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

#include "upnp/devicesmodel.h"
#include "upnp/device.h"
#include "upnp/localplaylists.h"
#include "core/configuration.h"
#include "core/debug.h"
#include "core/roles.h"
#include <QTimer>
#include <QCoreApplication>

static const int constTimeout=100;
static const char *constTimeoutProp="times";

Upnp::DevicesModel::DevicesModel(const QString &cfgName, HttpServer *h, QObject *parent)
    : QAbstractItemModel(parent)
    , http(h)
    , configName(cfgName)
    , timer(0)
{
    lastUuid=Core::Configuration(configName).get("lastUuid", QByteArray());
    if (!lastUuid.isEmpty()) {
        timer=new QTimer(this);
        connect(timer, SIGNAL(timeout()), SLOT(activateLast()));
        timer->setSingleShot(false);
        timer->start(constTimeout);
        timer->setProperty(constTimeoutProp, 0);
    }
}

Upnp::DevicesModel::~DevicesModel() {
    save();
    if (!devices.isEmpty()) {
        foreach (Device *dev, devices) {
            DBUG(Devices) << "unset" << (void *)dev << dev->uuid();
            dev->setActive(false);
        }

        devices.removeAll(LocalPlaylists::self());
        qDeleteAll(devices);
        devices.clear();
    }
}

void Upnp::DevicesModel::save() {
    for (int row=0; row<devices.count(); ++row) {
        Upnp::Device *dev=devices.at(row);
        if (dev->isActive()) {
            Core::Configuration(configName).set("lastUuid", dev->uuid());
            break;
        }
    }
}

QModelIndex Upnp::DevicesModel::index(int row, int col, const QModelIndex &parent) const {
    if (!hasIndex(row, col, parent)) {
        return QModelIndex();
    }

    return row<devices.count() ? createIndex(row, col, (void *)(devices.at(row))) : QModelIndex();
}

QModelIndex Upnp::DevicesModel::parent(const QModelIndex &index) const {
    Q_UNUSED(index)
    return QModelIndex();
}

int Upnp::DevicesModel::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : devices.count();
}

QVariant Upnp::DevicesModel::data(const QModelIndex &index, int role) const {
    const Device *device = toDevice(index);
    if (!device) {
        return QVariant();
    }
    
    switch (role) {
    case Core::Role_MainText:
    case Qt::DisplayRole:
        return device->name();
    case Core::Role_SubText:
        return device->host(); // +" ("+device->uuid()+")";
    case Qt::DecorationRole:
        return Device::monoIcon(device->icon());
    case Core::Role_IsCurrent:
        return device->isActive();
    case Qt::ToolTipRole:
        return device->name()+"<br/>"+device->host()+(device==LocalPlaylists::self() ? "" : ("<br/><i><small>"+device->uuid()+"</i></small>"));
    default:
        return QVariant();
    }
    return QVariant();
}

Qt::ItemFlags Upnp::DevicesModel::flags(const QModelIndex &index) const {
    if (index.isValid()) {
        return Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsEnabled;
    } else {
        return Qt::NoItemFlags;
    }
}

void Upnp::DevicesModel::added(const Ssdp::Device &device) {
    for (int i=0; i<devices.count() ; ++i) {
        Device *dev=devices.at(i);
        if (dev->uuid()==device.uuid) {
            return;
        }
    }

    Device *dev=createDevice(device);

    if (dev) {
        DBUG(Devices) << device.uuid << device.name << (void *)dev;
        beginInsertRows(QModelIndex(), devices.size(), devices.size());
        devices.append(dev);
        endInsertRows();
    }
}

void Upnp::DevicesModel::removed(const QByteArray &uuid) {
    DBUG(Devices) << uuid;
    for (int i=0; i<devices.count() ; ++i) {
        Device *dev=devices.at(i);
        if (dev->uuid()==uuid) {
            DBUG(Devices) << i;
            beginRemoveRows(QModelIndex(), i, i);
            devices.removeAt(i);
            endRemoveRows();
            dev->deleteLater();
            if (dev->isActive()) {
                DBUG(Devices) << "unset" << (void *)dev << dev->uuid();
                dev->setActive(false);
                if (devices.isEmpty()) {
                    setActiveDevice(0);
                } else {
                    setActiveDevice(devices.at(0));
                }
            }
            break;
        }
    }
}

void Upnp::DevicesModel::setActive(int row) {
    QObject *o=sender();
    DBUG(Devices) << (o? o->metaObject()->className() : "");
    for (int i=0; i<devices.count(); ++i) {
        Device *dev=devices.at(i);
        if (dev->isActive()) {
            if (i==row) {
                DBUG(Devices) << dev->uuid() << "Already active";
                return;
            } else {
                DBUG(Devices) << "unset" << (void *)dev << dev->uuid();
                QModelIndex idx=createIndex(i, 0, dev);
                dev->setActive(false);
                emit dataChanged(idx, idx);
                break;
            }
        }
    }
    if (row>=-0 && row<devices.count()) {
        if (timer) {
            timer->stop();
        }
        setActiveDevice(devices.at(row));
    } else {
        setActiveDevice(0);
    }
}

void Upnp::DevicesModel::notification(const QByteArray &sid, const QByteArray &data, int seq) {
    foreach (Device *dev, devices) {
        if (dev->isActive() && dev->hasSubscription(sid)) {
            dev->notification(sid, data, seq);
            return;
        }
    }
}

void Upnp::DevicesModel::connectionStateChanged(bool on) {
    if (on) {
        foreach (Device *dev, devices) {
            if (dev->isActive()) {
                dev->reset();
            }
        }
    }
}

void Upnp::DevicesModel::activateLast() {
    int activeRow=-1;

    if (lastUuid.isEmpty()) {
        activeRow=defaultActiveRow();
    } else {
        for (int row=0; row<devices.count() && -1==activeRow; ++row) {
            if (lastUuid==devices.at(row)->uuid()) {
                activeRow=row;
            }
        }
    }

    if (-1==activeRow) {
        if (timer->property(constTimeoutProp).toInt()<50) {
            timer->setProperty(constTimeoutProp, timer->property(constTimeoutProp).toInt()+1);
        } else {
            timer->stop();
            activeRow=defaultActiveRow();
        }
    }
    if (-1!=activeRow) {
        timer->stop();
        if (devices.count()>activeRow) {
            setActiveDevice(devices.at(activeRow));
        }
    }
}

void Upnp::DevicesModel::clear() {
    if (!devices.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, devices.count()-1);
        foreach (Device *dev, devices) {
            DBUG(Devices) << "unset" << (void *)dev << dev->uuid();
            dev->setActive(false);
        }

        qDeleteAll(devices);
        devices.clear();
        endRemoveRows();
        setActiveDevice(0);
    }
}

void Upnp::DevicesModel::setActiveDevice(Upnp::Device *dev) {
    if (!qApp) {
        return;
    }
    if (dev && dev->isActive()) {
        return;
    }
    Core::Configuration(configName).set("lastUuid", dev ? dev->uuid() : QString());
    if (dev) {
        DBUG(Devices) << (void *)dev << dev->uuid();
        dev->setActive(true);
        QModelIndex idx=createIndex(devices.indexOf(dev), 0, dev);
        emit activeDevice(idx);
        emit dataChanged(idx, idx);
    } else {
        DBUG(Devices) << "NONE";
        emit activeDevice(QModelIndex());
    }
}
