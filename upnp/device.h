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

#ifndef UPNP_DEVICE_H
#define UPNP_DEVICE_H

#include "upnp/ssdp.h"
#include "core/monoicon.h"
#include "core/utils.h"
#include "core/images.h"
#include <QAbstractItemModel>
#include <QHash>
#include <QUrl>

class QXmlStreamReader;
class QColor;
class QTimer;

namespace Core {
class NetworkJob;
}

namespace Upnp {
class DevicesModel;

class Device : public QAbstractItemModel {
    Q_OBJECT
public:
    enum Notifications {
        Notif_PlayCommand,
        Notif_SearchResult,

        Notif_Count
    };

    enum State {
        State_Initial,
        State_Populating,
        State_Populated
    };

    struct Item {
        enum Type {
            Type_MusicTrack = 0,
        };

        Item(const QString &n=QString(), Item *p=0, int r=0)
            : name(n), parent(p), row(r) { }
        virtual ~Item() { }
        virtual bool isCollection() const { return false; }
        virtual int type() const = 0;
        virtual QString mainText() const { return name; }
        virtual QString subText() const { return QString(); }
        virtual QString otherText() const { return QString(); }
        virtual Core::ImageDetails cover() const { return Core::ImageDetails(); }
        virtual Core::MonoIcon::Type icon() const { return Core::MonoIcon::music; }
        virtual QVariant actions() const { return QVariant(); }
        QString name;
        Item *parent;
        int row;
    };

    struct MusicTrack : public Item {
        MusicTrack(const QMap<QString, QString> &values, Item *p=0, int r=0);
        MusicTrack(const QString &n=QString(), Item *p=0, int r=0)
            : Item(n, p, r), isBroadcast(false), track(0), year(0), duration(0) { }
        virtual ~MusicTrack() { }
        virtual int type() const { return Type_MusicTrack; }
        virtual QString mainText() const;
        virtual QString subText() const { return 0==duration ? QString() : Core::Utils::formatTime(duration); }
        const QString & artistName() const { return albumArtist.isEmpty() ? artist : albumArtist; }
        virtual Core::ImageDetails cover() const;
        virtual Core::MonoIcon::Type icon() const { return isBroadcast ? Core::MonoIcon::ex_radio : Core::MonoIcon::music; }
        QByteArray toXml() const;
        bool isBroadcast;
        QString artistAndAlbum() const;
        QString describe() const;
        QString url;
        QString artist;
        QString albumArtist;
        QString album;
        QString creator;
        quint16 track;
        quint16 year;
        quint16 duration;
        QString genre;
        QString artUrl;
        QString date;
        QMap<QString, QString> res;
    };

public:
    static const char * constTrackClass;
    static const char * constBroadcastClass;
    static const char * constObjectIdListMimeType;
    static const char * constMsgTypeProperty;
    static const char * constMsgServiceProperty;
    static const int constNotifTimeout;
    static void setMonoIconCol(const QColor &col);
    static QIcon monoIcon(Core::MonoIcon::Type icon);

    Device(const Ssdp::Device &device, DevicesModel *parent = 0);
    virtual ~Device();
    virtual Core::MonoIcon::Type icon() const = 0;
    QModelIndex index(int, int, const QModelIndex & = QModelIndex()) const;
    QModelIndex parent(const QModelIndex &) const;
    int rowCount(const QModelIndex &parent = QModelIndex()) const;
    int columnCount(const QModelIndex &) const { return 1; }
    virtual QVariant data(const QModelIndex &index, int role) const;

    const QByteArray & uuid() const { return details.uuid; }
    const QString & name() const { return details.name; }
    const QString & host() const { return details.host; }
    bool isActive() const { return active; }
    bool hasSubscription(const QByteArray &sid) const { return -1!=subscriptions.values().indexOf(sid); }
    bool isEmpty() const { return items.isEmpty(); }
    int numItems() const { return items.count(); }
    void setLost(bool l) { wasLost=l; }

    virtual void clear();
    virtual void populate() = 0;
    virtual void notification(const QByteArray &sid, const QByteArray &data) = 0;

Q_SIGNALS:
    void stateChanged(const QString &str);
    void lost();
    void info(const QString &msg, quint8 id, int timeout=-1);

public Q_SLOTS:
    virtual void setActive(bool a);

protected:
    static QMap<QString, QString> objectValues(QXmlStreamReader &reader);

private Q_SLOTS:
    void jobFinished();
    void jobDestroyed();
    void subscriptionResponse();
    void otherResponse();
    void renewSubscriptions();

private:
    virtual void commandResponse(QXmlStreamReader &reader, const QByteArray &type, Core::NetworkJob *job) = 0;
    virtual void failedCommand(Core::NetworkJob *, const QByteArray &) { }

protected:
    Item * toItem(const QModelIndex &index) const { return index.isValid() ? static_cast<Item*>(index.internalPointer()) : 0; }
    Core::NetworkJob * sendCommand(const QByteArray &msg, const QByteArray &type, const QByteArray &service,
                                   bool cancelOthers=false, bool isPing=false);
    Core::NetworkJob * sendPingCommand(const QByteArray &msg, const QByteArray &type, const QByteArray &service)
        { return sendCommand(msg, type, service, false, true); }
    void cancelCommands(const QByteArray &type);
    void cancelAllJobs();
    void setState(State s);
    void requestSubscriptions();
    void cancelSubscriptions();

protected:
    QTimer *subTimer;
    DevicesModel *model;
    bool active;
    bool wasLost;
    Ssdp::Device details;
    QList<Item *> items;
    QList<Core::NetworkJob *> jobs;
    QHash<QUrl, QByteArray> subscriptions;
    State state;
};

}

#endif
