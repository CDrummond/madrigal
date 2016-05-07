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

#include "upnp/ssdp.h"
#include "core/debug.h"
#include "core/networkaccessmanager.h"
#include "config.h"
#include <QByteArray>
#include <QHostAddress>
#include <QUdpSocket>
#include <QNetworkConfigurationManager>
#include <QXmlStreamReader>
#include <QTimer>
#include <QFile>
#ifdef Q_OS_LINUX
#include <sys/types.h>
#include <sys/socket.h>
#endif

#define LINE_SEP "\r\n"

static const quint16 constPort = 1900;
static const char *constMulticastGroup = "239.255.255.250";
static const char * constUuidProp="uuid";
static const int constSearchPeriod=30*1000;
static const int constListingTimeout=10*1000;

static Core::MonoIcon::Type fontawesomeIcon(const QByteArray &name) {
    if (name=="chrome") {
        return Core::MonoIcon::chrome;
    }
    if (name=="play-circle-o") {
        return Core::MonoIcon::playcircleo;
    }
    if (name=="linux") {
        return Core::MonoIcon::fa_linux;
    }
    if (name=="music") {
        return Core::MonoIcon::music;
    }
    if (name=="android") {
        return Core::MonoIcon::android;
    }
    if (name=="apple") {
        return Core::MonoIcon::apple;
    }
    if (name=="windows") {
        return Core::MonoIcon::windows;
    }
    return Core::MonoIcon::no_icon;
}

static Core::MonoIcon::Type deviceIcon(const QByteArray &model) {
    static QMap<QByteArray, Core::MonoIcon::Type> iconMap;

    if (iconMap.isEmpty()) {
        QFile mapping(SYS_CONFIG_DIR+"mapping");
        if (mapping.open(QIODevice::ReadOnly|QIODevice::Text)) {
            while (!mapping.atEnd()) {
                QList<QByteArray> parts=mapping.readLine().trimmed().split('=');
                if (2==parts.length()) {
                    Core::MonoIcon::Type icon=fontawesomeIcon(parts[1]);
                    if (icon>Core::MonoIcon::no_icon) {
                        iconMap.insert(parts[0], icon);
                    }
                }
            }
        }
    }

    QMap<QByteArray, Core::MonoIcon::Type>::ConstIterator it=iconMap.constBegin();
    QMap<QByteArray, Core::MonoIcon::Type>::ConstIterator end=iconMap.constEnd();
    for (; it!=end; ++it) {
        if (-1!=model.indexOf(it.key())) {
            return it.value();
        }
    }
    return Core::MonoIcon::no_icon;
}

Upnp::Ssdp::Ssdp(QObject *p)
    : QObject(p)
    , network(0)
    , socket(0)
{
    QNetworkConfigurationManager *mgr=new QNetworkConfigurationManager(this);
    connect(mgr, SIGNAL(onlineStateChanged(bool)), this, SLOT(onlineStateChanged(bool)));
}

void Upnp::Ssdp::start() {
    if (socket) {
        return;
    }

    qRegisterMetaType<Device>("Ssdp::Device");
    qRegisterMetaType<Device::Service>("Ssdp::Device::Service");

    connectSocket();

    refreshTimer=new QTimer(this);
    connect(refreshTimer, SIGNAL(timeout()), SLOT(search()));
    refreshTimer->setSingleShot(false);
    refreshTimer->start(constSearchPeriod);

    listTimer=new QTimer(this);
    listTimer->setSingleShot(true);
    connect(listTimer, SIGNAL(timeout()), SLOT(listingTimeout()));

    search();
}

void Upnp::Ssdp::search() {
    // Discover all UPnP devices
    QByteArray request=QByteArray("M-SEARCH * HTTP/1.1"LINE_SEP"HOST: ")+QByteArray(constMulticastGroup)+
                       QByteArray(":")+QByteArray::number(constPort)+
                       QByteArray(LINE_SEP"MAN: \"ssdp:discover\""LINE_SEP"MX: 3\r\nST: upnp:rootdevice"LINE_SEP LINE_SEP);

    DBUG(Ssdp) << request;
    socket->writeDatagram(request, QHostAddress(constMulticastGroup), constPort);
    listTimer->start(constListingTimeout*0.8);
}

void Upnp::Ssdp::readDatagrams() {
    DBUG(Ssdp);

    while (socket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(socket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;

        socket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        bool isSearchResponse=datagram.startsWith("HTTP/1.1 200 OK");
        bool isNotify=datagram.startsWith("NOTIFY * HTTP/1.1");

        if (isSearchResponse || isNotify) {
            QList<QByteArray> parts=datagram.replace("\r", "").split('\n');
            QByteArray location;
            QByteArray uuid;
            bool isAlive=false;
            bool isByeBye=false;
            foreach (const QByteArray &part, parts) {
                QByteArray upper=part.toUpper();
                if (upper.startsWith("LOCATION: ")) {
                    location=part.mid(10);
                } else if (upper.startsWith("USN: ")) {
                    uuid=part.mid(5);
                } else if (isNotify && upper.startsWith("NTS: ")) {
                    if (upper.endsWith("SSDP:BYEBYE")) {
                        isByeBye=true;
                    } else if (upper.endsWith("SSDP:ALIVE")) {
                        isAlive=true;
                    }
                }
            }

            if (!uuid.isEmpty() && uuid.endsWith("::upnp:rootdevice")) {
                uuid=uuid.left(uuid.length()-17).mid(5);
                if (isSearchResponse) {
                    listedDevices.insert(uuid);
                }
                if ((isSearchResponse || isAlive) && !location.isEmpty() && !knownDevices.contains(uuid)) {
                    if (!network) {
                        network=new Core::NetworkAccessManager(this);
                    }
                    Core::NetworkJob *job=network->get(QUrl(location));
                    if (job) {
                        jobs.append(job);
                        job->setProperty(constUuidProp, uuid);
                        connect(job, SIGNAL(finished()), SLOT(jobFinished()));
                    }
                    knownDevices.insert(uuid);
                    DBUG(Ssdp) << "Read descr of" << uuid << location;
                } else if (isByeBye) {
                    knownDevices.remove(uuid);
                    DBUG(Ssdp) << "deviceRemoved (byebye)" << uuid;
                    emit deviceRemoved(uuid);
                }
            }
        }
    }
}

//#define DISPLAY_XML
void Upnp::Ssdp::jobFinished() {
    Core::NetworkJob *job=qobject_cast<Core::NetworkJob *>(sender());

    if (job) {
        job->deleteLater();
        if (0<=jobs.indexOf(job) && job->property(constUuidProp).isValid()) {
            #ifdef DISPLAY_XML
            QByteArray data=job->readAll();
            DBUG(Ssdp) << data;
            QXmlStreamReader reader(data);
            #else
            QXmlStreamReader reader(job->actualJob());
            #endif
            Device device;

            device.baseUrl=job->url().scheme().toLatin1()+"://"+job->url().host().toLatin1()+":"+QByteArray::number(job->url().port(80))+"/";
            device.host=QUrl(device.baseUrl).host();
            device.uuid=job->property(constUuidProp).toByteArray();
            device.icon=Core::MonoIcon::no_icon;

            reader.setNamespaceProcessing(false);
            while (!reader.atEnd()) {
                reader.readNext();

                if (QXmlStreamReader::StartElement==reader.tokenType()) {
                    if (QLatin1String("device")==reader.name()) {
                        while (!reader.atEnd()) {
                            reader.readNext();
                            if (QXmlStreamReader::StartElement==reader.tokenType()) {
                                if (QLatin1String("deviceType")==reader.name()) {
                                    device.type=reader.readElementText().toLatin1();
                                } else if (QLatin1String("friendlyName")==reader.name()) {
                                    device.name=reader.readElementText();
                                    if (device.name.endsWith(" (OpenHome)")) {
                                        device.name=device.name.left(device.name.length()-11);
                                    }
                                } else if (QLatin1String("manufacturer")==reader.name()) {
                                    device.manufacturer=reader.readElementText();
                                } else if (QLatin1String("modelName")==reader.name()) {
                                    device.icon=deviceIcon(reader.readElementText().toLatin1());
                                } else if (QLatin1String("serviceList")==reader.name()) {
                                    while (!reader.atEnd()) {
                                        reader.readNext();
                                        if (QXmlStreamReader::StartElement==reader.tokenType() && QLatin1String("service")==reader.name()) {
                                            Device::Service service;
                                            QByteArray type;
                                            while (!reader.atEnd()) {
                                                reader.readNext();
                                                if (QXmlStreamReader::StartElement==reader.tokenType()) {
                                                    if (QLatin1String("serviceType")==reader.name()) {
                                                        type=reader.readElementText().toLatin1();
                                                    } else if (QLatin1String("serviceId")==reader.name()) {
                                                        service.id=reader.readElementText().toLatin1();
                                                    } else if (QLatin1String("controlURL")==reader.name()) {
                                                        service.controlUrl=reader.readElementText().toLatin1();
                                                    } else if (QLatin1String("eventSubURL")==reader.name()) {
                                                        service.eventUrl=reader.readElementText().toLatin1();
                                                    }
                                                    // SCPDURL - url to XML service description
                                                } else if (QXmlStreamReader::EndElement==reader.tokenType() && QLatin1String("service")==reader.name()) {
                                                    break;
                                                }
                                            }
                                            if (!type.isEmpty() && !service.id.isEmpty()) {
                                                if (!service.controlUrl.isEmpty() && !service.controlUrl.startsWith('/')) {
                                                    service.controlUrl='/'+service.controlUrl;
                                                }
                                                if (!service.eventUrl.isEmpty() && !service.eventUrl.startsWith('/')) {
                                                    service.eventUrl='/'+service.eventUrl;
                                                }
                                                device.services.insert(type, service);
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    } else if (QLatin1String("URLBase")==reader.name()) {
                        device.baseUrl=reader.readElementText().toLatin1();
                    }
                }
            }

            if (device.baseUrl.endsWith('/') ){
                device.baseUrl=device.baseUrl.left(device.baseUrl.length()-1);
            }
            DBUG(Ssdp) << "deviceAdded" << device.uuid << device.type << device.services.keys();
            emit deviceAdded(device);
        }
        job->cancelAndDelete();
    }
}

void Upnp::Ssdp::listingTimeout() {
    QSet<QByteArray> removedDevices=knownDevices-listedDevices;
    DBUG(Ssdp) << "listed:" << listedDevices << "known:" << knownDevices << "removed:" << removedDevices;

    foreach (const QByteArray &uuid, removedDevices) {
        knownDevices.remove(uuid);
        DBUG(Ssdp) << "deviceRemoved" << uuid;
        emit deviceRemoved(uuid);
    }
    listedDevices.clear();
}

void Upnp::Ssdp::connectSocket() {
    if (socket) {
        disconnect(socket, SIGNAL(readyRead()), this, SLOT(readDatagrams()));
        socket->close();
        socket->deleteLater();
    }
    socket=new QUdpSocket(this);

    #ifdef Q_OS_LINUX
    // Workaround for https://bugreports.qt.io/browse/QTBUG-33419
    socket->setSocketDescriptor(::socket(PF_INET, SOCK_DGRAM, 0), QAbstractSocket::UnconnectedState);
    int reuse=1;
    ::setsockopt(socket->socketDescriptor(), SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));
    socket->bind(QHostAddress(QHostAddress::AnyIPv4), constPort, QUdpSocket::DefaultForPlatform);
    #else
    socket->bind(QHostAddress(QHostAddress::AnyIPv4), constPort, QUdpSocket::ReuseAddressHint|QUdpSocket::ShareAddress);
    #endif
    socket->joinMulticastGroup(QHostAddress(constMulticastGroup));
    connect(socket, SIGNAL(readyRead()), this, SLOT(readDatagrams()));
}

void Upnp::Ssdp::onlineStateChanged(bool on) {
    DBUG(Ssdp) << on;
    if (listTimer) {
        listTimer->stop();
    }
    if (on) {
        if (refreshTimer) {
            refreshTimer->start();
        }
        connectSocket();
        search();
    } else {
        foreach (const QByteArray &uuid, knownDevices) {
            DBUG(Ssdp) << "deviceRemoved" << uuid;
            emit deviceRemoved(uuid);
        }
        if (refreshTimer) {
            refreshTimer->stop();
        }
    }
    knownDevices.clear();
    listedDevices.clear();
}
