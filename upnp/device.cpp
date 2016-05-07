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

#include "upnp/device.h"
#include "upnp/httpserver.h"
#include "upnp/devicesmodel.h"
#include "core/debug.h"
#include "core/networkaccessmanager.h"
#include "core/monoicon.h"
#include "core/roles.h"
#include "config.h"
#include <QXmlStreamReader>
#include <QXmlStreamWriter>
#include <QTimer>

const char * Upnp::Device::constTrackClass="object.item.audioItem.musicTrack";
const char * Upnp::Device::constBroadcastClass="object.item.audioItem.audioBroadcast";
const char * Upnp::Device::constObjectIdListMimeType=APP_REV_URL"/track-list";
const char * Upnp::Device::constMsgServiceProperty="service";
const char * Upnp::Device::constMsgTypeProperty="type";
const int Upnp::Device::constNotifTimeout=2;
const char * constIsPingProperty="isping";
static const int constSubTimeout=1800;
static const int constSubRenewTimeout=1740;
static QColor monoIconColor=Qt::black;
QMap<Core::MonoIcon::Type, QIcon> monoIcons;

Upnp::Device::MusicTrack::MusicTrack(const QMap<QString, QString> &values, Item *p, int r)
    : Upnp::Device::Item(values["title"], p, r)
{
    isBroadcast=QLatin1String(constBroadcastClass)==values["class"];
    url=values["res"];
    artist=values["artist"];
    albumArtist=values["albumArtist"];
    creator=values["creator"];
    album=values["album"];
    genre=values["genre"];
    track=values["originalTrackNumber"].toUInt();
    artUrl=values["albumArtURI"];

    if (!isBroadcast && !name.isEmpty() && artist.isEmpty() && album.isEmpty() && 0==track && genre.isEmpty() && creator.isEmpty()) {
        isBroadcast=true;
    }
    if (artUrl.isEmpty()) {
        artUrl=Core::Images::self()->constDefaultImage;
    }
    if (values["date"].contains("-")) {
        year=values["date"].split("-").first().toUInt();
    }
    date=values["date"];
    duration=0;
    if (values.contains("res.duration")) {
        QStringList parts=values["res.duration"].split(":");
        if (!parts.isEmpty()) {
            quint16 multiple=1;
            for (int i=parts.size()-1; i>=0; --i) {
                duration+=parts.at(i).toDouble()*multiple;
                multiple*=60;
            }
        }
    }
}

Core::ImageDetails Upnp::Device::MusicTrack::cover() const {
    if (isBroadcast) {
        if (artUrl.isEmpty()) {
            return Core::ImageDetails();
        }
        return Core::ImageDetails(artUrl, name);
    }
    return Core::ImageDetails(artUrl, artistName().isEmpty() ? name : artistName(), album);
}

QString Upnp::Device::MusicTrack::mainText() const {
    return (track>0 ? (track>9 ? QString::number(track)+QLatin1Char(' ') : (QLatin1Char('0')+QString::number(track)+QLatin1Char(' '))) : QString())+
           name+
           (!albumArtist.isEmpty() && !artist.isEmpty() && artist!=albumArtist ? QLatin1String(" - ")+artist : QString());
}

QString Upnp::Device::MusicTrack::artistAndAlbum() const {
    if (album.isEmpty() && artist.isEmpty()) {
        return name.isEmpty() || isBroadcast ? QString() : QObject::tr("Unknown");
    } else if (album.isEmpty()) {
        return artist;
    } else {
        return artist+QLatin1String(" - ")+album; // TODO YEAR ??? displayAlbum());
    }
}

QString Upnp::Device::MusicTrack::describe() const {
    return artist.isEmpty()
            ? QObject::tr("%1 on %2").arg(name).arg(album)
            : QObject::tr("%1 by %2 on %3").arg(name).arg(artist).arg(album);
}

QByteArray Upnp::Device::MusicTrack::toXml() const {
    QByteArray xml;
    QXmlStreamWriter writer(&xml);

    writer.writeStartDocument();
    writer.writeStartElement(QLatin1String("DIDL-Lite"));
    writer.writeDefaultNamespace(QLatin1String("urn:schemas-upnp-org:metadata-1-0/DIDL-Lite/"));
    writer.writeNamespace(QLatin1String("http://purl.org/dc/elements/1.1/"), QLatin1String("dc"));
    writer.writeNamespace(QLatin1String("urn:schemas-upnp-org:metadata-1-0/upnp/"), QLatin1String("upnp"));
    writer.writeNamespace(QLatin1String("urn:schemas-dlna-org:metadata-1-0/"), QLatin1String("dlna"));
    writer.writeStartElement(QLatin1String("item"));
//    xml.writeAttribute(QLatin1String("id"), "TODO");
    if (!name.isEmpty()) {
        writer.writeStartElement(QLatin1String("dc:title"));
        writer.writeCharacters(name);
        writer.writeEndElement();
    }
    if (!artist.isEmpty()) {
        writer.writeStartElement(QLatin1String("dc:creator"));
        writer.writeCharacters(artist);
        writer.writeEndElement();
        writer.writeStartElement(QLatin1String("upnp:artist"));
        writer.writeCharacters(artist);
        writer.writeEndElement();
    }
    if (!albumArtist.isEmpty()) {
        writer.writeStartElement(QLatin1String("upnp:albumArtist"));
        writer.writeCharacters(albumArtist);
        writer.writeEndElement();
    }
    if (!album.isEmpty()) {
        writer.writeStartElement(QLatin1String("upnp:album"));
        writer.writeCharacters(album);
        writer.writeEndElement();
    }
    if (!artUrl.isEmpty()) {
        writer.writeStartElement(QLatin1String("upnp:albumArtURI"));
        writer.writeCharacters(artUrl);
        writer.writeEndElement();
    }
    if (track>0) {
        writer.writeStartElement(QLatin1String("upnp:originalTrackNumber"));
        writer.writeCharacters(QString::number(track));
        writer.writeEndElement();
    }
    if (!date.isEmpty()) {
        writer.writeStartElement(QLatin1String("dc:date"));
        writer.writeCharacters(date);
        writer.writeEndElement();
    }
    if (!genre.isEmpty()) {
        writer.writeStartElement(QLatin1String("upnp:genre"));
        writer.writeCharacters(genre);
        writer.writeEndElement();
    }

    if (!url.isEmpty()) {
        writer.writeStartElement(QLatin1String("res"));
        QMap<QString, QString>::ConstIterator it=res.constBegin();
        QMap<QString, QString>::ConstIterator end=res.constEnd();
        for (; it!=end; ++it) {
            writer.writeAttribute(it.key(), it.value());
        }
        writer.writeCharacters(url);
        writer.writeEndElement();
    }
    writer.writeStartElement(QLatin1String("upnp:class"));
    writer.writeCharacters(isBroadcast ? constBroadcastClass : constTrackClass);
    writer.writeEndElement();

    writer.writeEndElement();
    writer.writeEndElement();
    writer.writeEndDocument();
    return xml;
}

void Upnp::Device::setMonoIconCol(const QColor &col) {
    if (col!=monoIconColor) {
        monoIconColor=col;
        monoIcons.clear();
    }
}

QIcon Upnp::Device::monoIcon(Core::MonoIcon::Type icon) {
    QMap<Core::MonoIcon::Type, QIcon>::ConstIterator it=monoIcons.find(icon);
    if (monoIcons.end()!=it) {
        return it.value();
    }
    QIcon icn=Core::MonoIcon::icon(icon, monoIconColor, monoIconColor);
    monoIcons.insert(icon, icn);
    return icn;
}

Upnp::Device::Device(const Ssdp::Device &device, DevicesModel *parent)
    : QAbstractItemModel(parent)
    , subTimer(0)
    , model(parent)
    , active(false)
    , wasLost(false)
    , details(device)
    , state(State_Initial)
{
    if (DBUG_ENABLED(Devices)) {
        Ssdp::Device::Services::ConstIterator it=details.services.constBegin();
        Ssdp::Device::Services::ConstIterator end=details.services.constEnd();
        for(; it!=end; ++it) {
            DBUG(Devices) << "srvid:" << it.value().id << "type:" << it.key() << "control:" << it.value().controlUrl << "event:" << it.value().eventUrl;
        }
    }
}

Upnp::Device::~Device() {
    cancelAllJobs();
    cancelSubscriptions();
}

QModelIndex Upnp::Device::index(int row, int col, const QModelIndex &parent) const {
    if (!hasIndex(row, col, parent)) {
        return QModelIndex();
    }

    return row<items.count() ? createIndex(row, col, (void *)(items.at(row))) : QModelIndex();
}

QModelIndex Upnp::Device::parent(const QModelIndex &index) const {
    Q_UNUSED(index)
    return QModelIndex();
}

int Upnp::Device::rowCount(const QModelIndex &parent) const {
    return parent.isValid() ? 0 : items.count();
}

QVariant Upnp::Device::data(const QModelIndex &index, int role) const {
    const Item *item = toItem(index);

//    DBUG(Devices) << index.row() << (void *)item << role;
    if (!item) {
        return QVariant();
    }

    switch (role) {
    case Core::Role_MainText:
        return item->mainText();
    case Core::Role_SubText:
        return item->subText();
    case Core::Role_OtherText:
        return item->otherText();
    case Core::Role_ImageDetails: {
        QVariant var;
        var.setValue<Core::ImageDetails>(item->cover());
        return var;
    }
    case Core::Role_Actions:
        return item->actions();
    case Qt::DisplayRole:
        return item->mainText();
    case Qt::DecorationRole:
        return monoIcon(item->icon());
    default:
        break;
    }
    return QVariant();
}

void Upnp::Device::clear() {
    beginResetModel();
    qDeleteAll(items);
    items.clear();
    cancelAllJobs();
    endResetModel();
}

void Upnp::Device::setActive(bool a) {
    if (a==active) {
        return;
    }
    active=a;
    cancelAllJobs();
    if (active) {
        if (State_Initial==state) {
            setState(State_Populating);
            populate();
        }
        requestSubscriptions();
    } else {
        clear();
        setState(State_Initial);
        cancelSubscriptions();
    }
}

QMap<QString, QString> Upnp::Device::objectValues(QXmlStreamReader &reader) {
    QMap<QString, QString> values;
    QString elem=reader.name().toString();
    QXmlStreamAttributes attributes=reader.attributes();
    foreach (const QXmlStreamAttribute &attr, attributes) {
        values.insert(attr.name().toString(), attr.value().toString());
    }

    while (!reader.atEnd()) {
        reader.readNext();

        if (reader.isStartElement()) {
            QString key=reader.name().toString();
            if (QLatin1String("artist")==key) {
                QXmlStreamAttributes attributes=reader.attributes();
                if (attributes.value(QLatin1String("role"))==QLatin1String("AlbumArtist")) {
                    key=QLatin1String("albumArtist");
                }
            } else if (QLatin1String("res")==key) {
                QXmlStreamAttributes attributes=reader.attributes();
                foreach (const QXmlStreamAttribute &attr, attributes) {
                    values.insert("res."+attr.name().toString(), attr.value().toString());
                }
            }
            values.insert(key, reader.readElementText());
        } else if (reader.isEndElement() && elem==reader.name()) {
            break;
        }
    }
    return values;
}

Core::NetworkJob * Upnp::Device::sendCommand(const QByteArray &msg, const QByteArray &type, const QByteArray &service, bool cancelOthers, bool isPing) {
    Ssdp::Device::Services::ConstIterator srv=details.services.find(service);

    if (details.services.constEnd()!=srv) {
        if (cancelOthers) {
            cancelCommands(type);
        }

        QByteArray data="<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" "
                        "s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">"
                        "<s:Body><u:"+type+" xmlns:u=\""+service+"\">"+msg+"</u:"+type+"></s:Body></s:Envelope>";
        Core::NetworkAccessManager::RawHeaders headers;

        headers.insert("CONTENT-TYPE", "text/xml; charset=\"utf-8\"");
        headers.insert("SOAPACTION", "\""+service+"#"+type+"\"");
        Core::NetworkJob *job=Core::NetworkAccessManager::self()->post(QUrl(details.baseUrl+srv.value().controlUrl), data, headers, isPing ? 1000 : 0);
        connect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
        connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
        job->setProperty(constMsgTypeProperty, type);
        job->setProperty(constMsgServiceProperty, service);
        job->setProperty(constIsPingProperty, isPing);
        jobs.append(job);
        DBUG(Devices) << (void *)job << type << service;
        return job;
    }
    return 0;
}

void Upnp::Device::cancelCommands(const QByteArray &type) {
    QList<Core::NetworkJob *> toCancel;
    foreach (Core::NetworkJob *job, jobs) {
        if (job->property(constMsgTypeProperty).toByteArray()==type) {
            toCancel.append(job);
        }
    }
    foreach (Core::NetworkJob *job, toCancel) {
        disconnect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
        disconnect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
        job->cancelAndDelete();
        jobs.removeAll(job);
    }
}

//#define DISPLAY_XML
void Upnp::Device::jobFinished() {
    Core::NetworkJob *job=qobject_cast<Core::NetworkJob *>(sender());

    if (job) {
        jobs.removeAll(job);
        QByteArray msgType=job->property(constMsgTypeProperty).toByteArray();
        #ifdef DISPLAY_XML
        QByteArray data=job->readAll();
        DBUG(Devices) << (void *)job << data;
        QXmlStreamReader reader(data);
        #else
        QXmlStreamReader reader(job->actualJob());
        #endif
        while (!reader.atEnd()) {
            reader.readNext();
            if (QXmlStreamReader::StartElement==reader.tokenType() && QLatin1String("Envelope")==reader.name()) {
                while (!reader.atEnd()) {
                    reader.readNext();
                    if (QXmlStreamReader::StartElement==reader.tokenType() && QLatin1String("Body")==reader.name()) {
                        while (!reader.atEnd()) {
                            reader.readNext();
                            if (QXmlStreamReader::StartElement==reader.tokenType() && QLatin1String(msgType+"Response")==reader.name()) {
                                commandResponse(reader, msgType, job);
                                job->cancelAndDelete();
                                return;
                            }
                        }
                    }
                }
            }
        }

        failedCommand(job, msgType);
        if (job->property(constIsPingProperty).toBool()) {
            DBUG(Devices) << "LOST" << uuid();
            // If Device is LOST, then dont send unsubscribe messages.
            // Otherwise Qt complains with: "QNetworkReplyImplPrivate::error: Internal problem, this method must only be called once."
            wasLost=true;
            emit lost();
        }
        job->cancelAndDelete();
    }
}

void Upnp::Device::jobDestroyed() {
    Core::NetworkJob *job=qobject_cast<Core::NetworkJob *>(sender());
    if (job) {
        jobs.removeAll(job);
    }
}

void Upnp::Device::subscriptionResponse() {
    Core::NetworkJob *job=qobject_cast<Core::NetworkJob *>(sender());
    if (job) {
        QByteArray sid=job->actualJob()->rawHeader("SID");
        DBUG(Devices) << sid;
        if (!sid.isEmpty() && active) {
            subscriptions.insert(job->origUrl(), sid);
        }
        jobs.removeAll(job);
        job->cancelAndDelete();
    }
}

void Upnp::Device::otherResponse() {
    Core::NetworkJob *job=qobject_cast<Core::NetworkJob *>(sender());
    if (job) {
        jobs.removeAll(job);
        job->cancelAndDelete();
    }
}

void Upnp::Device::cancelAllJobs() {
    foreach (Core::NetworkJob *job, jobs) {
        disconnect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
        disconnect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
        job->cancelAndDelete();
    }
    jobs.clear();
}

void Upnp::Device::setState(State s) {
    if (s!=state) {
        state=s;
        switch (state) {
        case State_Initial:
            emit stateChanged(tr("Cleared"));
            break;
        case State_Populating:
            emit stateChanged(tr("Fetching items..."));
            break;
        case State_Populated:
            emit stateChanged(tr("All items fetched"));
            break;
        }
    }
}

void Upnp::Device::requestSubscriptions() {
    Ssdp::Device::Services::ConstIterator it=details.services.constBegin();
    Ssdp::Device::Services::ConstIterator end=details.services.constEnd();
    for(; it!=end; ++it) {
        QUrl url(details.baseUrl+it.value().eventUrl);
        Core::NetworkAccessManager::RawHeaders headers;
        //        headers["HOST"]="????";
        headers["CALLBACK"]="<http://"+model->httpServer()->getAddress(url)+":"+QByteArray::number(model->httpServer()->serverPort())+"/>";
        headers["NT"]="upnp:event";
        headers["TIMEOUT"]="Second-"+QByteArray::number(constSubTimeout);
        Core::NetworkJob *job=Core::NetworkAccessManager::self()->sendCustomRequest(url, "SUBSCRIBE", headers);
        connect(job, SIGNAL(finished()), this, SLOT(subscriptionResponse()));
        connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
        jobs.append(job);
    }
    if (!subTimer) {
        subTimer=new QTimer(this);
        connect(subTimer, SIGNAL(timeout()), this, SLOT(renewSubscriptions()));
        subTimer->setSingleShot(true);
    }
    if (!subTimer->isActive()) {
        subTimer->start(constSubRenewTimeout*1000);
    }
}

void Upnp::Device::renewSubscriptions() {
    if (active) {
        QHash<QUrl, QByteArray>::ConstIterator it=subscriptions.constBegin();
        QHash<QUrl, QByteArray>::ConstIterator end=subscriptions.constEnd();
        for(; it!=end; ++it) {
            QUrl url(it.key());
            Core::NetworkAccessManager::RawHeaders headers;
            //        headers["HOST"]="????";
            headers["SID"]=it.value();
            headers["TIMEOUT"]="Second-"+QByteArray::number(constSubTimeout);
            Core::NetworkJob *job=Core::NetworkAccessManager::self()->sendCustomRequest(url, "SUBSCRIBE", headers);
            connect(job, SIGNAL(finished()), this, SLOT(otherResponse()));
            connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
            jobs.append(job);
        }
        subTimer->start(constSubRenewTimeout*1000);
    }
}

void Upnp::Device::cancelSubscriptions() {
    if (wasLost) {
        return;
    }
    if (subTimer) {
        subTimer->stop();
    }
    QHash<QUrl, QByteArray>::ConstIterator it=subscriptions.constBegin();
    QHash<QUrl, QByteArray>::ConstIterator end=subscriptions.constEnd();
    for(; it!=end; ++it) {
        QUrl url(it.key());
        Core::NetworkAccessManager::RawHeaders headers;
        //        headers["HOST"]="????";
        headers["SID"]=it.value();
        Core::NetworkJob *job=Core::NetworkAccessManager::self()->sendCustomRequest(url, "UNSUBSCRIBE", headers);
        connect(job, SIGNAL(finished()), this, SLOT(otherResponse()));
        connect(job, SIGNAL(destroyed(QObject*)), this, SLOT(jobDestroyed()));
        jobs.append(job);
    }
    active=false;
    subscriptions.clear();
}
