/*
 * Cantata
 *
 * Copyright (c) 2011-2016 Craig Drummond <craig.p.drummond@gmail.com>
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

#include "core/monoicon.h"
#include "core/utils.h"
#include <QFile>
#include <QIconEngine>
#include <QSvgRenderer>
#include <QPainter>
#include <QPixmapCache>
#include <QRect>
#include <QFontDatabase>
#include <QApplication>

namespace Core {

class MonoIconEngine : public QIconEngine {
public:
    MonoIconEngine(const QString &file, MonoIcon::Type t, const QColor &col, const QColor &sel)
        : fileName(file)
        , type(t)
        , color(col)
        , selectedColor(sel)
    {
        switch (type) {
        case MonoIcon::ex_cd:            fileName=":cd"; break;
        case MonoIcon::ex_mediaplay:     fileName=QApplication::isLeftToRight() ? ":media-play" : ":media-play-rtl"; break;
        case MonoIcon::ex_mediapause:    fileName=":media-pause"; break;
        case MonoIcon::ex_mediaprevious: fileName=":media-prev"; break;
        case MonoIcon::ex_medianext:     fileName=":media-next";  break;
        case MonoIcon::ex_mediastop:     fileName=":media-stop"; break;
        case MonoIcon::ex_radio:         fileName=":radio"; break;
        default: break;
        }
    }

    virtual ~MonoIconEngine() {}

    MonoIconEngine * clone() const {
        return new MonoIconEngine(fileName, type, color, selectedColor);
    }

    virtual void paint(QPainter *painter, const QRect &rect, QIcon::Mode mode, QIcon::State state) {
        Q_UNUSED(state)

        QColor col=QIcon::Selected==mode ? selectedColor : color;
        QString key=(fileName.isEmpty() ? QString::number(type) : fileName)+
                    QLatin1Char('-')+QString::number(rect.width())+QLatin1Char('-')+QString::number(rect.height())+QLatin1Char('-')+col.name();
        QPixmap pix;

        if (!QPixmapCache::find(key, &pix)) {
            pix=QPixmap(rect.width(), rect.height());
            pix.fill(Qt::transparent);
            QPainter p(&pix);

            if (fileName.isEmpty()) {
                QString fontName;
                double scale=0.9;
                int yAdjust=0;
                if (MonoIcon::ex_one==type) {
                    fontName="serif";
                } else {
                    // Load fontawesome, if it is not already loaded
                    if (fontAwesomeFontName.isEmpty()) {
                        QFile fa(":fa.otf");
                        fa.open(QIODevice::ReadOnly);
                        QStringList loadedFontFamilies = QFontDatabase::applicationFontFamilies(QFontDatabase::addApplicationFontFromData(fa.readAll()));
                        if (!loadedFontFamilies.empty()) {
                            fontAwesomeFontName= loadedFontFamilies.at(0);
                        }
                        fa.close();
                    }

                    switch (type) {
                    case MonoIcon::lastfmsquare:
                    case MonoIcon::lastfm:
                        scale=1.1;
                        break;
                    case MonoIcon::list:
                        scale=1.05;
                        break;
                    case MonoIcon::bars:
                        if (rect.height()>18 ) {
                            scale=0.95;
                        }
                        break;
                    case MonoIcon::retweet:
                        yAdjust=-1;
                        break;
                    default:
                        break;
                    }
                    fontName=fontAwesomeFontName;
                }

                QFont font(fontName);
                int pixelSize=qRound(rect.height()*scale);
                if (MonoIcon::ex_one==type) {
                    font.setBold(true);
                } else if (pixelSize>=12 && pixelSize<=16 && rect.height()>14) {
                    pixelSize=14;
                } else if (pixelSize>=24 && pixelSize<=32 && rect.height()>28) {
                    pixelSize=28;
                }

                font.setPixelSize(pixelSize);
                font.setStyleStrategy(QFont::PreferAntialias);
                font.setHintingPreference(QFont::PreferFullHinting);
                p.setFont(font);
                p.setPen(col);
                p.setRenderHint(QPainter::Antialiasing, true);
                if (MonoIcon::ex_one==type) {
                    QString str=QString::number(type);
                    p.drawText(QRect(0, 0, rect.width(), rect.height()), str, QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
                    p.drawText(QRect(1, 0, rect.width(), rect.height()), str, QTextOption(Qt::AlignHCenter|Qt::AlignVCenter));
                } else {
                    p.drawText(QRect(0, yAdjust, rect.width(), rect.height()), QString(QChar(static_cast<int>(type))), QTextOption(Qt::AlignCenter|Qt::AlignVCenter));
                }
            } else {
                QSvgRenderer renderer;
                QFile f(rect.width()<=32 && QFile::exists(fileName+"32") ? fileName+"32" : fileName);
                QByteArray bytes;
                if (f.open(QIODevice::ReadOnly)) {
                    bytes=f.readAll();
                }
                if (!bytes.isEmpty()) {
                    bytes.replace("#000", col.name().toLatin1());
                }
                renderer.load(bytes);
                renderer.render(&p, QRect(0, 0, rect.width(), rect.height()));
            }
            QPixmapCache::insert(key, pix);
        }
        if (QIcon::Disabled==mode) {
            painter->save();
            painter->setOpacity(painter->opacity()*0.35);
        }
        painter->drawPixmap(rect.topLeft(), pix);
        if (QIcon::Disabled==mode) {
            painter->restore();
        }
    }

    virtual QPixmap pixmap(const QSize &size, QIcon::Mode mode, QIcon::State state) {
        QPixmap pix(size);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        paint(&painter, QRect(QPoint(0, 0), size), mode, state);
        return pix;
    }

private:
    QString fileName;
    MonoIcon::Type type;
    QColor color;
    QColor selectedColor;
    static QString fontAwesomeFontName;
};

QString MonoIconEngine::fontAwesomeFontName;

QIcon MonoIcon::icon(const QString &fileName, const QColor &col, const QColor &sel) {
    return QIcon(new MonoIconEngine(fileName, (MonoIcon::Type)0, col, sel));
}

QIcon MonoIcon::icon(Type icon, const QColor &col, const QColor &sel) {
    return QIcon(new MonoIconEngine(QString(), icon, col, sel));
}

}