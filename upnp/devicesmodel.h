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

#ifndef UPNP_DEVICES_MODEL_H
#define UPNP_DEVICES_MODEL_H

#include "upnp/ssdp.h"
#include <QAbstractItemModel>
#include <QTimer>

namespace Upnp {

class Device;
class HttpServer;

class DevicesModel : public QAbstractItemModel {
    Q_OBJECT

public:
    DevicesModel(const QString &cfgName, HttpServer *h, QObject *parent=0);
    virtual ~DevicesModel();
    void save();
    QModelIndex index(int, int, const QModelIndex & = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &) const { return 1; }
    QVariant data(const QModelIndex &, int) const;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    bool isInitialising() const { return timer && timer->isActive(); }

    HttpServer *httpServer() { return http; }

Q_SIGNALS:
    void activeDevice(const QModelIndex &idx);

public Q_SLOTS:
    virtual void added(const Ssdp::Device &device);
    void removed(const QByteArray &uuid);
    void setActive(int row);
    void notification(const QByteArray &sid, const QByteArray &data);

private Q_SLOTS:
    void activateLast();

private:
    void clear();
    virtual Device * createDevice(const Ssdp::Device &device)=0;
    virtual int defaultActiveRow() const { return 0; }
    void setActiveDevice(Device *dev);
    Device * toDevice(const QModelIndex &index) const { return index.isValid() ? static_cast<Device*>(index.internalPointer()) : 0; }

protected:
    HttpServer *http;
    QString configName;
    QList<Device *> devices;
    QByteArray lastUuid;
    QTimer *timer;
};

}

#endif
