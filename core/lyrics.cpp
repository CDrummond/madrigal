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

#include "core/lyrics.h"
#include "core/networkaccessmanager.h"
#include "core/debug.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include <QUrl>
#include <QUrlQuery>
#include <QXmlStreamReader>
#include <QTextCodec>

static const char * constSearchProp="search";

static QString extract(const QString &source, const QString &begin, const QString &end, bool isTag=false) {
    DBUGF(Lyrics) << "Looking for" << begin << end;
    int beginIdx = source.indexOf(begin, 0, Qt::CaseInsensitive);
    bool skipTagClose=false;

    if (-1==beginIdx && isTag) {
        beginIdx = source.indexOf(QString(begin).remove(">"), 0, Qt::CaseInsensitive);
        skipTagClose=true;
    }
    if (-1==beginIdx) {
        DBUGF(Lyrics) << "Failed to find begin";
        return QString();
    }
    if (skipTagClose) {
        int closeIdx=source.indexOf(">", beginIdx);
        if (-1!=closeIdx) {
            beginIdx=closeIdx+1;
        } else {
            beginIdx += begin.length();
        }
    } else {
        beginIdx += begin.length();
    }

    int endIdx = source.indexOf(end, beginIdx, Qt::CaseInsensitive);
    if (-1==endIdx && QLatin1String("null")!=end) {
        DBUGF(Lyrics) << "Failed to find end";
        return QString();
    }

    DBUGF(Lyrics) << "Found match";
    return source.mid(beginIdx, endIdx - beginIdx - 1);
}


Core::Lyrics::Lyrics(QObject *p)
    : QObject(p)
    , enabled(false)
    , job(0)
    , renderer(0)
{
    connect(Upnp::Model::self()->renderersModel(), SIGNAL(activeDevice(QModelIndex)), this, SLOT(setRenderer(QModelIndex)));
}

void Core::Lyrics::setEnabled(bool en) {
    if (en!=enabled) {
        enabled=en;
        if (enabled) {
            update(songWhenDisabled);
            songWhenDisabled=Upnp::Device::MusicTrack();
        }
    }
}

void Core::Lyrics::setRenderer(Upnp::Renderer *r) {
    if (r==renderer) {
        return;
    }
    if (renderer) {
        disconnect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
    }
    renderer=r;
    if (renderer) {
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(update(QModelIndex)));
        update(renderer->current());
    } else {
        update(QModelIndex());
    }
}

void Core::Lyrics::setRenderer(const QModelIndex &idx) {
    setRenderer(static_cast<Upnp::Renderer *>(idx.internalPointer()));
}

void Core::Lyrics::update(const QModelIndex &idx) {
    if (idx.isValid()) {
        update(*static_cast<Upnp::Device::MusicTrack *>(idx.internalPointer()));
    } else {
        update(Upnp::Device::MusicTrack());
    }
}

void Core::Lyrics::update(const Upnp::Device::MusicTrack &song) {
    if (!enabled) {
        songWhenDisabled=song;
        return;
    }
    if (song.artist!=currentSong.artist || song.name!=currentSong.name) {
        cancel();
        currentSong = song;
        if (!currentSong.artist.isEmpty() && !currentSong.name.isEmpty()) {
            QUrl url("http://lyrics.wikia.com/api.php");
            QUrlQuery query;

            query.addQueryItem(QLatin1String("artist"), currentSong.basicArtist());
            query.addQueryItem(QLatin1String("song"), currentSong.name);
            query.addQueryItem(QLatin1String("func"), QLatin1String("getSong"));
            query.addQueryItem(QLatin1String("fmt"), QLatin1String("xml"));
            url.setQuery(query);

            job = NetworkAccessManager::self()->get(url);
            job->setProperty(constSearchProp, true);
            DBUG(Lyrics) << url.toString();
            connect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
            emit fetched(currentSong.artist, currentSong.name, tr("Fetching..."));
        } else {
            emit fetched(currentSong.artist, currentSong.name, QString());
        }
    }
}

void Core::Lyrics::jobFinished() {
    NetworkJob *reply = qobject_cast<NetworkJob*>(sender());
    if (!reply) {
        return;
    }

    DBUG(Lyrics) << (void *)reply << (void *)job << reply->ok() << reply->property(constSearchProp).toBool() << reply->errorString();

    if (reply==job) {
        job=0;
        if (reply->ok()) {
            if (reply->property(constSearchProp).toBool()) {
                searchResponse(reply);
            } else {
                lyricsResponse(reply);
            }
        } else {
            emit fetched(currentSong.artist, currentSong.name, tr("No lyrics found"));
        }
    }
    reply->cancelAndDelete();
}

void Core::Lyrics::cancel() {
    if (job) {
        disconnect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
        job->cancelAndDelete();
        job=0;
    }
}

void Core::Lyrics::searchResponse(NetworkJob *reply) {
    DBUG(Lyrics);
    QUrl url;
    QXmlStreamReader doc(reply->actualJob());
    while (!doc.atEnd()) {
        doc.readNext();
        if (doc.isStartElement() && QLatin1String("url")==doc.name()) {
            QString lyricsUrl=doc.readElementText();
            if (!lyricsUrl.contains(QLatin1String("action=edit"))) {
                url=QUrl::fromEncoded(lyricsUrl.toUtf8()).toString();
            }
            break;
        }
    }

    if (url.isValid()) {
        QString path=url.path();
        QByteArray u=url.scheme().toLatin1()+"://"+url.host().toLatin1()+"/api.php?action=query&prop=revisions&rvprop=content&format=xml&titles=";
        QByteArray titles=QUrl::toPercentEncoding(path.startsWith(QLatin1Char('/')) ? path.mid(1) : path).replace('+', "%2b");
        job = NetworkAccessManager::self()->get(QUrl::fromEncoded(u+titles));
        DBUG(Lyrics) << (u+titles);
        connect(job, SIGNAL(finished()), this, SLOT(jobFinished()));
    } else {
        emit fetched(currentSong.artist, currentSong.name, tr("No lyrics found"));
    }
}

void Core::Lyrics::lyricsResponse(NetworkJob *reply) {
    const QTextCodec *codec = QTextCodec::codecForName("utf-8");
    QString contents = codec->toUnicode(reply->readAll()).replace("<br />", "<br/>");
    DBUG(Lyrics) << contents;

    contents=extract(contents, QLatin1String("&lt;lyrics&gt;"), QLatin1String("&lt;/lyrics&gt;")).trimmed();
    contents=contents.replace("\n\n\n", "\n\n");
    if (contents.isEmpty()) {
        emit fetched(currentSong.artist, currentSong.name, tr("No lyrics found"));
    } else {
        emit fetched(currentSong.artist, currentSong.name, contents);
    }
}
