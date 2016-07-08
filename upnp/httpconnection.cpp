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

#include "upnp/httpconnection.h"
#include "core/debug.h"
#include <QTimer>

static const int constMaxLen=32768;

Upnp::HttpConnection::HttpConnection(qintptr socketDescriptor, QObject *p)
    : QTcpSocket(p)
    , headersReceived(false)
    , chunked(false)
    , allChunksReceived(false)
    , size(0)
{
    setSocketDescriptor(socketDescriptor);
    timer=new QTimer(this);
    connect(timer, SIGNAL(timeout()), SLOT(timeout()));
    connect(this, SIGNAL(readyRead()), SLOT(readData()));
    timer->start(2000);
}

Upnp::HttpConnection::~HttpConnection() {
    DBUG(Http);
}

void Upnp::HttpConnection::close() {
    deleteLater();
    QTcpSocket::close();
}

void Upnp::HttpConnection::timeout() {
    DBUG(Http);
    close();
}

void Upnp::HttpConnection::readData() {
    while (bytesAvailable()) {
        if (headersReceived) {
            if (chunked) {
                QByteArray data = readLine().replace("\n\r", "").trimmed();
                if (!data.isEmpty()) {
                    bool ok=false;
                    qint64 len=data.toInt(&ok, 16);
                    if (!ok || len<0 || len>constMaxLen) {
                        DBUG(Http) << "Invalid chunk len" << len << ok;
                        close();
                        return;
                    }
                    if (0==len) {
                        allChunksReceived=true;
                    } else {
                        QByteArray newData=read(len);
                        if (newData.length()!=len) {
                            DBUG(Http) << "Failed to read chunk";
                            close();
                            return;
                        }
                        body+=newData;
                    }
                }
            } else {
                body+=readAll();
            }
            if ( (chunked && allChunksReceived) || (!chunked && (body.size()>=size || (body.size()==size-1 && body.at(body.size()-1)=='>')))) {
                DBUG(Http) << "finished" << body.size() << size;
                emit notification(headers["SID"], body.replace("\r", ""), headers["SEQ"].toUInt());
                QByteArray resp="HTTP/1.1 200 OK\r\nCONNECTION: close\r\nCONTENT-LENGTH: 41\r\nCONTENT-TYPE: text/html\r\n\r\n"
                                "<html><body><h1>200 OK</h1></body></html>";
                write(resp);
                close();
            }
        } else {
            QByteArray data = readLine().trimmed();
            if (headers.isEmpty() && !data.startsWith("NOTIFY / HTTP/1.1")) {
                DBUG(Http) << "Message is not a notification";
                close();
                return;
            }
            int colon = data.indexOf(':');
            if (colon > 0) {
                currentHeader = data.left(colon).toUpper();
                headers.insert(currentHeader, data.mid(colon+1).trimmed());
                continue;
            }

            if (colon <= 0 && !data.isEmpty()) {
                headers.insert(currentHeader, headers.value(currentHeader)+" "+data);
                continue;
            }

            if (data.isEmpty()) {
                QByteArray contentType = headers.value("CONTENT-TYPE");

                if (-1==contentType.indexOf("text/xml")) {
                    DBUG(Http) << "Wrong content type" << contentType;
                    close();
                    return;
                }

                if ("CHUNKED"==headers.value("TRANSFER-ENCODING").toUpper()) {
                    chunked=true;
                } else {
                    size = headers.value("CONTENT-LENGTH").toInt();
                    if (size <= 0 || size> constMaxLen) {
                        DBUG(Http) << "Invalid size" << size;
                        close();
                        return;
                    }
                }

                if (!headers.contains("SID")) {
                    DBUG(Http) << "No SID";
                    close();
                    return;
                }
                DBUG(Http) << "Have headers";
                headersReceived=true;
            }
        }
    }
}
