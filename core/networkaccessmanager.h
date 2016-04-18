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

#ifndef CORE_NETWORK_ACCESS_MANAGER_H
#define CORE_NETWORK_ACCESS_MANAGER_H

#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QMap>
#include <QTimer>

class QTimerEvent;
class AccessManager;

namespace Core {

class NetworkJob : public QObject {
    Q_OBJECT
public:
    NetworkJob(QNetworkReply *j);
    virtual ~NetworkJob();

    QNetworkReply * actualJob() const { return job; }

    void cancelAndDelete();
    bool open(QIODevice::OpenMode mode) { return job && job->open(mode); }
    void close() { if (job) job->close(); }

    QUrl url() const { return job ? job->url() : origU; }
    QUrl origUrl() const { return origU; }
    void setOrigUrl(const QUrl &u) { origU=u; }
    QNetworkReply::NetworkError error() const { return job ? job->error() : QNetworkReply::UnknownNetworkError; }
    QString errorString() const { return job ? job->errorString() : QString(); }
    QByteArray readAll() { return job ? job->readAll() : QByteArray(); }
    bool ok() const { return job && QNetworkReply::NoError==job->error(); }
    QVariant attribute(QNetworkRequest::Attribute code) const { return job ? job->attribute(code) : QVariant(); }
    qint64 bytesAvailable() const { return job ? job->bytesAvailable() : -1; }
    QByteArray read(qint64 maxlen) { return job ? job->read(maxlen) : QByteArray(); }
    void setMaxRedirects(int m) { maxRedir=m; }
    virtual int maxRedirects() const { return maxRedir; }

Q_SIGNALS:
    void finished();
    void error(QNetworkReply::NetworkError);
    void uploadProgress(qint64 bytesSent, qint64 bytesTotal);
    void downloadProgress(qint64 bytesReceived, qint64 bytesTotal);
    void downloadPercent(int pc);
    void readyRead();

private Q_SLOTS:
    void jobFinished();
    void jobDestroyed(QObject *o);
    void downloadProg(qint64 bytesReceived, qint64 bytesTotal);
    void handleReadyRead();

protected:
    void connectJob();
    void cancelJob();
    void abortJob();

private:
    int maxRedir;
    int numRedirects;
    int lastDownloadPc;
    QNetworkReply *job;
    QUrl origU;

    friend class NetworkAccessManager;
};

class NetworkAccessManager : public QNetworkAccessManager
{
    Q_OBJECT

public:
    static void enableDebug();
    static NetworkAccessManager * self();

    typedef QMap<QByteArray, QByteArray> RawHeaders;

    NetworkAccessManager(QObject *parent=0);
    virtual ~NetworkAccessManager() { }

    NetworkJob * get(const QNetworkRequest &req, int timeout=0);
    NetworkJob * get(const QUrl &url, int timeout=0) { return get(QNetworkRequest(url), timeout); }
    NetworkJob * post(QNetworkRequest req, const QByteArray &data, const RawHeaders &rawHeaders=RawHeaders(), int timeout=0);
    NetworkJob * post(const QUrl &url, const QByteArray &data, const RawHeaders &rawHeaders=RawHeaders(), int timeout=0)
        { return post(QNetworkRequest(url), data, rawHeaders, timeout); }
    NetworkJob * sendCustomRequest(QNetworkRequest req, const QByteArray &verb, const RawHeaders &rawHeaders=RawHeaders());
    NetworkJob * sendCustomRequest(const QUrl &req, const QByteArray &verb, const RawHeaders &rawHeaders=RawHeaders())
        { return sendCustomRequest(QNetworkRequest(req), verb, rawHeaders); }
    void setEnabled(bool e) { enabled=e; }
    bool isEnabled() const { return enabled; }

protected:
    void timerEvent(QTimerEvent *e);

private Q_SLOTS:
    void replyFinished();

private:
    void stopTimer(NetworkJob *job);

private:
    bool enabled;
    QMap<NetworkJob *, int> timers;
    friend class NetworkJob;
};

}

#endif // NETWORK_ACCESS_MANAGER_H
