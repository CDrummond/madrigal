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

#include "core/networkaccessmanager.h"
#include "core/globalstatic.h"
#include "core/debug.h"
#include <QTimerEvent>
#include <QTimer>
#include <QSslSocket>
#include <QCoreApplication>
#include <QBuffer>

namespace Core {

class NoRedirectNetworkJob : public NetworkJob {
public:
    NoRedirectNetworkJob(QNetworkReply *j)
        : NetworkJob(j) { }
    virtual ~NoRedirectNetworkJob() { }
    virtual int maxRedirects() const { return 0; }
};

}

Core::NetworkJob::NetworkJob(QNetworkReply *j)
    : QObject(j->parent())
    , maxRedir(5)
    , numRedirects(0)
    , lastDownloadPc(0)
    , job(j)
{
    origU=j->url();
    connectJob();
    DBUG(Network) << (void *)this << (void *)job << origU;
}

Core::NetworkJob::~NetworkJob() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    cancelJob();
}

void Core::NetworkJob::cancelAndDelete() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    cancelJob();
    deleteLater();
}

void Core::NetworkJob::connectJob() {
    if (!job) {
        return;
    }

    connect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
    connect(job, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
    connect(job, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(error(QNetworkReply::NetworkError)));
    connect(job, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));
    connect(job, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProg(qint64, qint64)));
    connect(job, SIGNAL(destroyed(QObject *)), this, SLOT(jobDestroyed(QObject *)));
}

void Core::NetworkJob::cancelJob() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    if (job) {
        QNetworkAccessManager *mgr=job->manager();
        if (mgr && qobject_cast<NetworkAccessManager *>(mgr)) {
            static_cast<NetworkAccessManager *>(mgr)->stopTimer(this);
        }
        disconnect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
        disconnect(job, SIGNAL(readyRead()), this, SLOT(handleReadyRead()));
        disconnect(job, SIGNAL(error(QNetworkReply::NetworkError)), this, SIGNAL(error(QNetworkReply::NetworkError)));
        disconnect(job, SIGNAL(uploadProgress(qint64, qint64)), this, SIGNAL(uploadProgress(qint64, qint64)));
        disconnect(job, SIGNAL(downloadProgress(qint64, qint64)), this, SLOT(downloadProg(qint64, qint64)));
        disconnect(job, SIGNAL(destroyed(QObject *)), this, SLOT(jobDestroyed(QObject *)));
        job->close();
        job->abort();
        job->deleteLater();
        job=0;
    }
}

void Core::NetworkJob::abortJob() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    if (job) {
        job->abort();
    }
}

void Core::NetworkJob::jobFinished() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    if (!job) {
        emit finished();
    }

    QNetworkReply *j=qobject_cast<QNetworkReply *>(sender());
    if (!j || j!=job) {
        return;
    }

    QVariant redirect = j->header(QNetworkRequest::LocationHeader);
    if (redirect.isValid() && ++numRedirects<maxRedirects()) {
        QNetworkReply *newJob=static_cast<QNetworkAccessManager *>(j->manager())->get(j->request());
        DBUG(Network) << j->url().toString() << "redirected to" << newJob->url().toString();
        cancelJob();
        job=newJob;
        connectJob();
        return;
    }

    DBUG(Network) << job->url().toString() << job->error() << (0==job->error() ? QLatin1String("OK") : job->errorString());
    emit finished();
}

void Core::NetworkJob::jobDestroyed(QObject *o) {
    DBUG(Network) << (void *)this << (void *)job << origU;
    if (o==job) {
        job=0;
    }
}

void Core::NetworkJob::downloadProg(qint64 bytesReceived, qint64 bytesTotal) {
    int pc=((bytesReceived*1.0)/(bytesTotal*1.0)*100.0)+0.5;
    pc=pc<0 ? 0 : (pc>100 ? 100 : pc);
    if (pc!=lastDownloadPc) {
        emit downloadPercent(pc);
    }
    emit downloadProgress(bytesReceived, bytesTotal);
}

void Core::NetworkJob::handleReadyRead() {
    DBUG(Network) << (void *)this << (void *)job << origU;
    QNetworkReply *j=dynamic_cast<QNetworkReply *>(sender());
    if (!j || j!=job) {
        return;
    }
    if (j->attribute(QNetworkRequest::RedirectionTargetAttribute).isValid()) {
        return;
    }
    emit readyRead();
}

GLOBAL_STATIC(Core::NetworkAccessManager, instance)

Core::NetworkAccessManager::NetworkAccessManager(QObject *parent)
    : QNetworkAccessManager(parent)
{
}

Core::NetworkJob * Core::NetworkAccessManager::get(const QNetworkRequest &req, int timeout) {
    DBUG(Network) << req.url().toString();

    QNetworkRequest request=req;
    request.setRawHeader("Connection", "close");

    // Windows builds do not support HTTPS - unless QtNetwork is recompiled...
    NetworkJob *reply=0;
    if (QLatin1String("https")==req.url().scheme() && !QSslSocket::supportsSsl()) {
        QUrl httpUrl=request.url();
        httpUrl.setScheme(QLatin1String("http"));
        request.setUrl(httpUrl);
        DBUG(Network) << "no ssl, use" << httpUrl.toString();
        reply = new NetworkJob(QNetworkAccessManager::get(request));
        reply->setOrigUrl(req.url());
    } else {
        reply = new NetworkJob(QNetworkAccessManager::get(request));
    }

    if (0!=timeout) {
        connect(reply, SIGNAL(destroyed()), SLOT(replyFinished()));
        connect(reply, SIGNAL(finished()), SLOT(replyFinished()));
        timers[reply] = startTimer(timeout);
    }
    return reply;
}

Core::NetworkJob * Core::NetworkAccessManager::post(QNetworkRequest req, const QByteArray &data, const RawHeaders &rawHeaders, int timeout) {
    DBUG(Network) << req.url().toString() << data.length() << rawHeaders;
    RawHeaders::ConstIterator it=rawHeaders.constBegin();
    RawHeaders::ConstIterator end=rawHeaders.constEnd();
    for (; it!=end; ++it) {
        req.setRawHeader(it.key(), it.value());
    }
//    req.setRawHeader("Connection", "close");
    NetworkJob *reply=new NoRedirectNetworkJob(QNetworkAccessManager::post(req, data));
    if (0!=timeout) {
        connect(reply, SIGNAL(destroyed()), SLOT(replyFinished()));
        connect(reply, SIGNAL(finished()), SLOT(replyFinished()));
        timers[reply] = startTimer(timeout);
    }
    return reply;
}

Core::NetworkJob * Core::NetworkAccessManager::sendCustomRequest(QNetworkRequest req, const QByteArray &verb,
                                                                 const RawHeaders &rawHeaders) {
    DBUG(Network) << req.url().toString() << verb;
    RawHeaders::ConstIterator it=rawHeaders.constBegin();
    RawHeaders::ConstIterator end=rawHeaders.constEnd();
    for (; it!=end; ++it) {
        req.setRawHeader(it.key(), it.value());
    }
    req.setRawHeader("Connection", "close");

    return new NoRedirectNetworkJob(QNetworkAccessManager::sendCustomRequest(req, verb));
}

void Core::NetworkAccessManager::replyFinished() {
    NetworkJob *job = qobject_cast<NetworkJob *>(sender());
    DBUG(Network) << (void *)job;
    stopTimer(job);
}

void Core::NetworkAccessManager::timerEvent(QTimerEvent *e) {
    NetworkJob *job = timers.key(e->timerId());
    DBUG(Network) << (void *)job;
    if (job) {
        stopTimer(job);
        job->abortJob();
    }
}

void Core::NetworkAccessManager::stopTimer(NetworkJob *job) {
    if (timers.contains(job)) {
        killTimer(timers.take(job));
    }
}
