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

#ifndef CORE_IMAGES_H
#define CORE_IMAGES_H

#include <QObject>
#include <QImage>
#include <QCache>
#include <QSet>
#include <QHash>

namespace Core {
class NetworkAccessManager;
class NetworkJob;
class Thread;

struct ImageDetails {
    ImageDetails(const QString &u=QString(), const QString &ar=QString(), const QString &al=QString())
        : url(u), artist(ar), album(al) { }
    bool operator==(const ImageDetails &o) const { return o.url==url; }
    QString url;
    QString artist;
    QString album;
};

class ImageLocator : public QObject {
    Q_OBJECT
public:
    struct Item {
        Item(const ImageDetails &d=ImageDetails(), int s=0) : details(d), size(s) { }
        ImageDetails details;
        int size;
    };

    struct Image : public Item {
        Image(QImage *i=0, const ImageDetails &d=ImageDetails(), int s=0) : Item(d, s), img(i) { }
        QImage *img;
    };

    struct Job {
        Job(NetworkJob *j=0, int s=0) : netJob(j) {
            if (0!=s) {
                sizes.insert(s);
            }
        }

        NetworkJob *netJob;
        QSet<int> sizes;
    };

    ImageLocator();
    void stop();

public Q_SLOTS:
    void load(const ImageDetails &details, int size);

Q_SIGNALS:
    void loaded(const QList<ImageLocator::Image> &items);

private Q_SLOTS:
    void load();
    void jobFinished();

private:
    void startTimer(int interval);

private:
    NetworkAccessManager *network;
    Thread *thread;
    QTimer *timer;
    QList<Item> queue;
    QHash<ImageDetails, Job *> jobs;
};

class Images : public QObject {
    Q_OBJECT
public:
    static const char * constCdCover;
    static const char * constCacheFilename;
    static Images * self();

    Images();
    QImage *get(const ImageDetails &details, int size, bool urgent=false);
    void clearDiskCache();

Q_SIGNALS:
    void found(const Core::ImageDetails &image);
    void load(const ImageDetails &derails, int size);

private Q_SLOTS:
    void loaded(const QList<ImageLocator::Image> &items);

private:
    QCache<QString, QImage> cache;
    ImageLocator *loader;
};

inline uint qHash(const Core::ImageDetails &key) {
    return qHash(key.url);
}

}

Q_DECLARE_METATYPE(Core::ImageDetails)

#endif
