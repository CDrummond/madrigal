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

#include "ui/groupeditemdelegate.h"
#include "ui/listitemdelegate.h"
#include "ui/gtkstyle.h"
#include "ui/utils.h"
#include "upnp/device.h"
#include "core/utils.h"
#include "core/images.h"
#include "core/roles.h"
#ifdef Q_OS_MAC
#include "mac/osxstyle.h"
#endif
#include <QApplication>
#include <QPainter>

static int coverSize=-1;
static int borderSize=1;

static const double constTextSizeAdjust=1.25;

static bool isHeader(const QModelIndex &index) {
    QModelIndex prev=index.row()>0 ? index.sibling(index.row()-1, 0) : QModelIndex();
    if (!prev.isValid()) {
        return true;
    }
    Upnp::Device::MusicTrack *thisSong=static_cast<Upnp::Device::MusicTrack *>(index.internalPointer());
    Upnp::Device::MusicTrack *prevSong=static_cast<Upnp::Device::MusicTrack *>(prev.internalPointer());

    return thisSong->artistName()!=prevSong->artistName() || thisSong->album!=prevSong->album;
}

static quint32 totalDuration(const QModelIndex &index) {
    Upnp::Device::MusicTrack *currentSong=static_cast<Upnp::Device::MusicTrack *>(index.internalPointer());
    quint32 total=currentSong->duration;
    QModelIndex next=index.sibling(index.row()+1, 0);
    while (next.isValid()) {
        Upnp::Device::MusicTrack *song=static_cast<Upnp::Device::MusicTrack *>(next.internalPointer());
        if (!song || song->artistName()!=currentSong->artistName() || song->album!=currentSong->album || song->isBroadcast!=currentSong->isBroadcast) {
            break;
        }
        total+=song->duration;
        next=next.sibling(next.row()+1, 0);
    }
    return total;
}

Ui::GroupedItemDelegate::GroupedItemDelegate(QObject *p)
    : Ui::BasicItemDelegate(p)
{

}

Ui::GroupedItemDelegate::~GroupedItemDelegate() {

}

QSize Ui::GroupedItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (-1==coverSize || -1==borderSize) {
        ListItemDelegate::getSizes(borderSize, coverSize);
    }
    int textHeight = QApplication::fontMetrics().height()*constTextSizeAdjust;
    if (isHeader(index)) {
        return QSize(Utils::scaleForDpi(64), qMax(coverSize, (qMax(Utils::scaleForDpi(22), textHeight)*2)+borderSize)+(2*borderSize));
    }
    return QSize(Utils::scaleForDpi(64), qMax(Utils::scaleForDpi(22), textHeight)+(2*borderSize));
}

void Ui::GroupedItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (!index.isValid()) {
        return;
    }

    bool isHdr=isHeader(index);
    Upnp::Device::MusicTrack *song=static_cast<Upnp::Device::MusicTrack *>(index.internalPointer());
    bool isCurrent=index.data(Core::Role_IsCurrent).toBool();
    bool selected=option.state&QStyle::State_Selected;
    bool active=option.state&QStyle::State_Active;
    bool mouseOver=underMouse && option.state&QStyle::State_MouseOver;
    bool gtk=mouseOver && GtkStyle::isActive();
    bool rtl=QApplication::isRightToLeft();
    QPalette::ColorGroup colorGroup=active ? QPalette::Active : QPalette::Inactive;

    if (isHdr) {
        if (mouseOver && gtk) {
            GtkStyle::drawSelection(option, painter, (selected ? 0.75 : 0.25)*0.75);
        } else {
            painter->save();
            painter->setOpacity(0.75);
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, qobject_cast<QWidget *>(parent()));
            painter->restore();
        }
        painter->save();
        painter->setClipRect(option.rect.adjusted(0, option.rect.height()/2, 0, 0), Qt::IntersectClip);
        if (mouseOver && gtk) {
            GtkStyle::drawSelection(option, painter, selected ? 0.75 : 0.25);
        } else {
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, qobject_cast<QWidget *>(parent()));
        }
        painter->restore();
    } else {
        if (mouseOver && gtk) {
            GtkStyle::drawSelection(option, painter, selected ? 0.75 : 0.25);
        } else {
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &option, painter, qobject_cast<QWidget *>(parent()));
        }
    }
    QString title;
    QString track=song->mainText();
    QString duration=song->duration>0 ? Core::Utils::formatTime(song->duration) : QString();
    QFont f(QApplication::font());
    QFontMetrics fm(f);
    int textHeight=fm.height()*constTextSizeAdjust;

    if (isHdr) {
        title=song->artistName();
        if (!song->album.isEmpty()) {
            title+=QLatin1String(" - ")+song->album;
        } else if (title.isEmpty() && song->isBroadcast) {
            title=QObject::tr("Stream");
        }
    }

    painter->save();
    painter->setFont(f);
    QTextOption textOpt(Qt::AlignVCenter);
    QRect r(option.rect.adjusted(borderSize+4, borderSize, -(borderSize+4), -borderSize));

    if (isCurrent) {
        QRectF border(option.rect.x()+1.5, option.rect.y()+1.5, option.rect.width()-3, option.rect.height()-3);
        if (!title.isEmpty()) {
            border.adjust(0, (border.height()/2)+1, 0, 0);
        }
        #ifdef Q_OS_MAC
        QColor gradCol(Mac::OSXStyle::self()->viewPalette().color(QPalette::Highlight));
        QColor borderCol(Mac::OSXStyle::self()->viewPalette().color(selected ? QPalette::HighlightedText : QPalette::Highlight));
        #else
        QColor gradCol(QApplication::palette().color(colorGroup, QPalette::Highlight));
        QColor borderCol(QApplication::palette().color(colorGroup, selected ? QPalette::HighlightedText : QPalette::Highlight));
        #endif
        if (!selected) {
            borderCol.setAlphaF(0.5);
        }
        gradCol.setAlphaF(selected ? 0.4 : 0.25);
        painter->setRenderHint(QPainter::Antialiasing, true);
        int radius=Utils::scaleForDpi(3);
        painter->fillPath(Utils::buildPath(border, radius), gradCol);
        painter->setPen(QPen(borderCol, 1));
        painter->drawPath(Utils::buildPath(border, radius));
        painter->setRenderHint(QPainter::Antialiasing, false);
    }

    #ifdef Q_OS_WIN
    QColor textColor(option.palette.color(colorGroup, QPalette::Text));
    #else
    QColor textColor(option.palette.color(colorGroup, option.state&QStyle::State_Selected ? QPalette::HighlightedText : QPalette::Text));
    #endif
    painter->setPen(textColor);
    bool showTrackDuration=!duration.isEmpty();

    if (isHdr) {
        Core::ImageDetails cover=index.data(Core::Role_ImageDetails).value<Core::ImageDetails>();
        QImage *img=cover.url.isEmpty() ? 0 : Core::Images::self()->get(cover, coverSize);
        QPixmap pix;
        if (img) {
            pix=QPixmap::fromImage(*img);
        }

        #if QT_VERSION >= 0x050100
        int maxSize=coverSize*pix.devicePixelRatio();
        #else
        int maxSize=coverSize;
        #endif

        if (pix.width()>maxSize) {
            pix=pix.scaled(maxSize, maxSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        }

        #if QT_VERSION >= 0x050100
        QSize pixSize = pix.isNull() ? QSize(0, 0) : (pix.size() / pix.devicePixelRatio());
        #else
        QSize pixSize = pix.size();
        #endif

        if (rtl) {
            painter->drawPixmap(r.x()+r.width()-(pixSize.width()-borderSize), r.y()+((r.height()-pixSize.height())/2), pixSize.width(), pixSize.height(), pix);
            r.adjust(0, 0, -(coverSize+borderSize), 0);
        } else {
            painter->drawPixmap(r.x()-2, r.y()+((r.height()-pixSize.height())/2), pixSize.width(), pixSize.height(), pix);
            r.adjust(coverSize+borderSize, 0, 0, 0);
        }

        int td=totalDuration(index);
        QString totalDuration=td>0 && td!=song->duration ? Core::Utils::formatTime(td) : QString();
        QRect duratioRect(r.x(), r.y(), r.width(), r.height()/2);
        int totalDurationWidth=fm.width(totalDuration)+8;
        QRect textRect(r.x(), r.y(), r.width()-(rtl ? (4*borderSize) : totalDurationWidth), r.height()/2);
        QFont tf(f);
        tf.setBold(true);
        title = QFontMetrics(tf).elidedText(title, Qt::ElideRight, textRect.width(), QPalette::WindowText);
        painter->setFont(tf);
        painter->drawText(textRect, title, textOpt);
        if (!totalDuration.isEmpty()) {
            painter->drawText(duratioRect, totalDuration, QTextOption(Qt::AlignVCenter|Qt::AlignRight));
        }
        BasicItemDelegate::drawLine(painter, r.adjusted(0, 0, 0, -r.height()/2), textColor, rtl, !rtl, 0.45);
        r=QRect(r.x(), r.y()+(r.height()/2)+1, r.width(), r.height()/2);
        painter->setFont(f);
        if (rtl) {
            r.adjust(0, 0, (coverSize-(borderSize*3)), 0);
        }
    } else if (rtl) {
        r.adjust(0, 0, -(borderSize*4), 0);
    } else {
        r.adjust(coverSize+borderSize, 0, 0, 0);
    }

    painter->setPen(textColor);

    int durationWidth=showTrackDuration ? fm.width(duration)+8 : 0;
    QRect textRect(r.x(), r.y(), r.width()-durationWidth, r.height());
    track = fm.elidedText(track, Qt::ElideRight, textRect.width(), QPalette::WindowText);
    painter->drawText(textRect, track, textOpt);
    if (showTrackDuration) {
        painter->drawText(r, duration, QTextOption(Qt::AlignVCenter|Qt::AlignRight));
    }

    if (isCurrent) {
        ListItemDelegate::drawPlayState(painter, option, textRect, index.data(Core::Role_PlayState).toInt());
    }
    BasicItemDelegate::drawLine(painter, option.rect, textColor);
    painter->restore();
}
