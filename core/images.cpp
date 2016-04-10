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

#include "core/images.h"
#include "core/networkaccessmanager.h"
#include "core/debug.h"
#include "core/globalstatic.h"
#include "core/utils.h"
#include "config.h"

GLOBAL_STATIC(Core::Images, instance)

const char * Core::Images::constCdCover=PACKAGE_NAME"://cd-cover";
const char * Core::Images::constStreamCover=PACKAGE_NAME"://stream-cover";
const char * Core::Images::constCacheFilename="cache-filename";

static const QString constCacheDir=QLatin1String("images/");
static const int constMaxCoverSize=400;

static QString cacheKey(const Core::ImageDetails &cover, int size, bool isStream=false) {
    return (isStream ? Core::Images::constStreamCover : (cover.artist+"-"+cover.album))+"-"+QString::number(size);
}

static QString fixString(QString str) {
    static const QChar constReplacement('_');
    static const QList<QChar> constRemove=QList<QChar>() << QLatin1Char('/') << QLatin1Char('#') << QLatin1Char('?')
                                                         << QLatin1Char('*')  << QLatin1Char('\\') << QLatin1Char(':');
    foreach (const QChar &ch, constRemove) {
        str=str.replace(ch, constReplacement);
    }
    return str;
}

static QString diskCacheName(const Core::ImageDetails &cov, bool createDir=true) {
    if (cov.album.isEmpty()) {
        return Core::Utils::cacheDir(constCacheDir, createDir)+fixString(cov.artist);
    }
    return Core::Utils::cacheDir(constCacheDir+fixString(cov.artist), createDir)+fixString(cov.album);
}

static bool isJpg(const QByteArray &data)
{
    return data.size()>9 && /*data[0]==0xFF && data[1]==0xD8 && data[2]==0xFF*/ data[6]=='J' && data[7]=='F' && data[8]=='I' && data[9]=='F';
}

static bool isPng(const QByteArray &data)
{
    return data.size()>4 && /*data[0]==0x89 &&*/ data[1]=='P' && data[2]=='N' && data[3]=='G';
}

static const char * typeFromRaw(const QByteArray &raw)
{
    if (isJpg((raw))) {
        return "JPG";
    } else if (isPng(raw)) {
        return "PNG";
    }
    return 0;
}

static QImage loadImage(const QString &fileName)
{
    QImage img(fileName);
    if (img.isNull()) {
        // Failed to load, perhaps extension is wrong? If so try PNG for .jpg, and vice versa...
        QFile f(fileName);
        if (f.open(QIODevice::ReadOnly)) {
            QByteArray header=f.read(10);
            f.reset();
            img.load(&f, typeFromRaw(header));
        }
    }
    return img;
}

static QImage cachedImage(const Core::ImageDetails &cover) {
    QString cacheName=diskCacheName(cover, false);
    foreach (const QString &type, QStringList() << ".png" << ".jpg") {
        if (QFile::exists(cacheName+type)) {
            QImage i=loadImage(cacheName+type);
            if (!i.isNull()) {
                i.setText(Core::Images::constCacheFilename, cacheName+type);
                DBUGF(Images) << cover.album << cover.artist << i.width() << QString(cacheName+type);
                return i;
            }
        }
    }
    return QImage();
}

Core::ImageLocator::ImageLocator()
    : network(0)
    , timer(0)
{
    thread=new Thread(metaObject()->className());
    moveToThread(thread);
    thread->start();
}

void Core::ImageLocator::stop() {
    thread->stop();
}

void Core::ImageLocator::startTimer(int interval) {
    if (!timer) {
        timer=thread->createTimer(this);
        timer->setSingleShot(true);
        connect(timer, SIGNAL(timeout()), this, SLOT(load()), Qt::QueuedConnection);
    }
    timer->start(interval);
}

void Core::ImageLocator::load(const ImageDetails &details, int size) {
    queue.append(Item(details, size));
    startTimer(0);
}

void Core::ImageLocator::load() {
    QList<Item> toDo;
    for (int i=0; i<5 && !queue.isEmpty(); ++i) {
        toDo.append(queue.takeFirst());
    }
    if (toDo.isEmpty()) {
        return;
    }
    QList<Image> covers;
    foreach (const Item &i, toDo) {
        DBUG(Images) << i.details.artist << i.details.album << i.details.url << i.size;
        QImage img=cachedImage(i.details);
        if (!img.isNull()) {
            DBUG(Images) << i.details.artist << i.details.album << i.details.url << i.size << "Got from cache";
            covers.append(Image(new QImage(img.scaled(i.size, i.size, Qt::KeepAspectRatio, Qt::SmoothTransformation)), i.details, i.size));
            continue;
        }

        DBUG(Images) << i.details.artist << i.details.album << i.details.url << i.size << "Download";
        QHash<ImageDetails, Job *>::iterator it=jobs.find(i.details);
        if (it==jobs.end()) {
            if (!network) {
                network=new NetworkAccessManager(this);
            }
            Job *job=new Job(network->get(i.details.url), i.size);
            job->sizes.insert(0);
            connect(job->netJob, SIGNAL(finished()), this, SLOT(jobFinished()));
            jobs.insert(i.details, job);
        } else {
            DBUG(Images) << "Already downloading";
            it.value()->sizes.insert(i.size);
        }
    }
    if (!covers.isEmpty()) {
        DBUG(Images) << "loaded" << covers.count();
        emit loaded(covers);
    }

    if (!queue.isEmpty()) {
        startTimer(0);
    }
}

void Core::ImageLocator::jobFinished() {
    NetworkJob *j=qobject_cast<NetworkJob *>(sender());
    DBUG(Images);

    if (j) {
        QHash<ImageDetails, Job *>::iterator it=jobs.begin();
        QHash<ImageDetails, Job *>::iterator end=jobs.end();

        for (; it!=end; ++it) {
            if (it.value()->netJob==j) {
                DBUG(Images) << "found job";
                QList<Image> images;
                QByteArray data=j->readAll();
                QImage img;
                const char *type=0;
                if (!data.isEmpty()) {
                    type=typeFromRaw(data);
                    img.loadFromData(data, type);
                }

                if (img.isNull()) {
                    DBUG(Images) << it.key().url << "NULL";
                } else {
                    DBUG(Images) << it.key().url << img.height();
                    if (img.height()>constMaxCoverSize ||img.width()>constMaxCoverSize) {
                        DBUG(Images) << "scale as too large";
                        img=img.scaled(constMaxCoverSize, constMaxCoverSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                        type="PNG";
                    }
                    QString name=diskCacheName(it.key(), true)+(QLatin1String("PNG")==type ? ".png" : ".jpg");
                    DBUG(Images) << "save" << name;
                    img.save(name);
                    img.setText(Images::constCacheFilename, name);
                    foreach (int size, it.value()->sizes) {
                        if (0!=size) {
                            QImage *scaled=new QImage(img.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                            scaled->setText(Images::constCacheFilename, name);
                            images.append(Image(scaled, it.key(), size));
                        }
                    }
                }
                if (!images.isEmpty()) {
                    emit loaded(images);
                }
                delete it.value();
                jobs.erase(it);
                break;
            }
        }

        j->cancelAndDelete();
    }
}

Core::Images::Images()
    : cache(10*1024*1024)
    , loader(0)
{
    qRegisterMetaType<ImageDetails>("ImageDetails");
    qRegisterMetaType<ImageLocator::Image>("ImageLocator::Image");
    qRegisterMetaType<QList<ImageLocator::Image> >("QList<ImageLocator::Image>");
}

QImage * Core::Images::get(const ImageDetails &details, int size, bool urgent) {
    DBUG(Images) << details.artist << details.album << size << details.url;
    if (!details.url.isEmpty() && !details.artist.isEmpty() && details.url!=constCdCover && details.url!=constStreamCover) {
        // Check memory cache...
        QString key=cacheKey(details, size);
        QImage *img=cache.object(key);
        if (img && img->width()>1) {
            DBUG(Images) << details.artist << details.album << size << key << "found in cache";
            return img;
        }

        // Check for original size in cache...
        img=cache.object(cacheKey(details, 0));
        if (img) {
            if (img->width()>1) {
                img=new QImage(img->scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                cache.insert(key, img, img->width()*img->height()*8);
                DBUG(Images) << details.artist << details.album << size << key << cacheKey(details, 0) << "found in 0 cache";
                return img;
            }
            img=0;
        }

        if (!img) {
            if (urgent) {
                QImage ci=cachedImage(details);
                if (!ci.isNull()) {
                    img=new QImage(0==size ? ci : ci.scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
                    cache.insert(key, img, img->width()*img->height());
                    DBUG(Images) << details.artist << details.album << size << key << "got from disk cache";
                    return img;
                }
            }

            if (!loader) {
                loader=new ImageLocator();
                connect(loader, SIGNAL(loaded(QList<ImageLocator::Image>)), this, SLOT(loaded(QList<ImageLocator::Image>)), Qt::QueuedConnection);
                connect(this, SIGNAL(load(ImageDetails,int)), loader, SLOT(load(ImageDetails,int)), Qt::QueuedConnection);
            }
            DBUG(Images) << details.artist << details.album << size << key << cacheKey(details, 0) << "load";
            emit load(details, size);
            // Insert dummy entry into cache...
            cache.insert(key, new QImage(1, 1, QImage::Format_Mono), 1);
        }
    }
    // Return default cover...
    // TODO: Artist images?
    bool isStream=details.url==constStreamCover;
    QString defKey=cacheKey(ImageDetails(), size, isStream);
    QImage *defImg=cache.object(defKey);
    if (defImg) {
        DBUG(Images) << details.artist << details.album << size << defKey << "use default";
        return defImg;
    }

    defImg=new QImage(QImage(isStream ? ":stream" : size<=32 ? ":optical32" : ":optical").scaled(size, size, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    cache.insert(defKey, defImg, defImg->width()*defImg->height());
    DBUG(Images) << details.artist << details.album << size << defKey << "use newly created default";
    return defImg;
}

void Core::Images::clearDiskCache() {
    QString dir=Utils::cacheDir(constCacheDir, false);
    if (!dir.isEmpty()) {
        Utils::clearFolder(dir, QStringList() << "*.jpg" << "*.png");
    }
}

void Core::Images::loaded(const QList<ImageLocator::Image> &items) {
    QSet<ImageDetails> foundDetails;
    foreach (const ImageLocator::Image &item, items) {
        if (item.img) {
            QString key=cacheKey(item.details, item.size);
            if (cache.contains(key)) {
                cache.remove(key);
            }
            DBUG(Images) << "insert" << key << item.img->text(constCacheFilename);
            cache.insert(key, item.img, item.img->width()*item.img->height());
            foundDetails.insert(item.details);
        }
    }
    foreach (const ImageDetails &f, foundDetails) {
        emit found(f);
    }
}
