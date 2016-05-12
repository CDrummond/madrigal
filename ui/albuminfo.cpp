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

#include "ui/albuminfo.h"
#include "ui/toolbutton.h"
#include "ui/squeezedtextlabel.h"
#include "ui/utils.h"
#include "ui/viewtoolbar.h"
#include <QGridLayout>
#include <QVBoxLayout>

Ui::AlbumInfo::AlbumInfo(QWidget *p)
    : QWidget(p)
{
    QGridLayout *layout=new QGridLayout(this);
    QWidget *controls=new QWidget(this);
    QVBoxLayout *controlLayout=new QVBoxLayout(controls);
    layout->setMargin(Utils::scaleForDpi(2));
    layout->setSpacing(Utils::scaleForDpi(2));
    controlLayout->setMargin(0);
    controlLayout->setSpacing(0);
    name=new SqueezedTextLabel(this);
    artist=new SqueezedTextLabel(this);
    details=new SqueezedTextLabel(this);
    cover=new QLabel(this);
    playButton=new ToolButton(controls);
    addButton=new ToolButton(controls);
    controlLayout->addWidget(playButton);
    controlLayout->addWidget(addButton);
    layout->addWidget(cover, 0, 0, 5, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum), 0, 1, 1, 1);
    layout->addWidget(name, 1, 1, 1, 1);
    layout->addWidget(artist, 2, 1, 1, 1);
    layout->addWidget(details, 3, 1, 1, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::Minimum), 4, 1, 1, 1);
    layout->addWidget(controls, 0, 2, 5, 1);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

    QFont nameFont(artist->font());
    nameFont.setBold(true);
    name->setFont(nameFont);
    QFont detailsFont(Utils::smallFont(artist->font()));
    details->setFont(detailsFont);
    QPalette pal=name->palette();
    QColor col(name->palette().windowText().color());
    col.setAlphaF(0.5);
    pal.setColor(QPalette::WindowText, col);
    details->setPalette(pal);

    ensurePolished();
    int h=QFontMetrics(name->font()).height()+QFontMetrics(artist->font()).height()+QFontMetrics(details->font()).height()+(layout->spacing()*3);
    cover->setFixedSize(h, h);
    connect(Core::Images::self(), SIGNAL(found(Core::ImageDetails)), this, SLOT(coverLoaded(Core::ImageDetails)));
    connect(playButton, SIGNAL(clicked(bool)), this, SIGNAL(play()));
    connect(addButton, SIGNAL(clicked(bool)), this, SIGNAL(add()));

    QColor iconCol=Utils::clampColor(palette().foreground().color());
    playButton->setIcon(Core::MonoIcon::icon(Core::MonoIcon::ex_mediaplay, iconCol, iconCol));
    addButton->setIcon(Core::MonoIcon::icon(Core::MonoIcon::plus, iconCol, iconCol));
    int iconSize=Ui::ViewToolBar::iconSize();
    int buttonSize=Ui::ViewToolBar::buttonSize();
    playButton->setIconSize(QSize(iconSize, iconSize));
    addButton->setIconSize(QSize(iconSize, iconSize));
    playButton->setFixedSize(QSize(buttonSize, buttonSize));
    addButton->setFixedSize(QSize(buttonSize, buttonSize));
    playButton->setToolTip(Core::Actions::toolTip(Core::Actions::Action_Play));
    addButton->setToolTip(Core::Actions::toolTip(Core::Actions::Action_Add));
}

void Ui::AlbumInfo::update(const Upnp::MediaServer::Album *info) {
    if (info) {
        QString currentAlbum=name->fullText();
        QString currentArtist=artist->fullText();
        name->setText(info->name);
        artist->setText(info->artist);

        quint32 dur=0;
        quint32 num=info->children.count();

        foreach (Upnp::Device::Item *child, info->children) {
            dur+=static_cast<Upnp::MediaServer::Track *>(child)->duration;
        }

        if (0==num) {
            details->setText(tr("No Tracks"));
        } else if (1==num) {
            details->setText(tr("1 Track (%1)").arg(Core::Utils::formatDuration(dur)));
        } else {
            details->setText(tr("%1 Tracks (%2)").arg(num).arg(Core::Utils::formatDuration(dur)));
        }

        if (currentAlbum!=info->name || currentArtist!=info->artist) {
            updateCover(info);
        }
    } else {
        name->setText(QString());
        artist->setText(QString());
        details->setText(QString());
        updateCover(0);
    }
}

void Ui::AlbumInfo::updateCover(const Upnp::MediaServer::Album *info) {
    QImage *img=0;
    if (info) {
        img=Core::Images::self()->get(info->cover(), cover->height());
    } else {
        img=Core::Images::self()->get(Core::ImageDetails(), cover->height());
    }

    if (img) {
        cover->setPixmap(QPixmap::fromImage(*img));
    } else {
        cover->setPixmap(QPixmap());
    }
}

void Ui::AlbumInfo::coverLoaded(const Core::ImageDetails &image) {
    if (image.artist==artist->fullText() && image.album==name->fullText()) {
        QImage *img=Core::Images::self()->get(image, cover->height(), true);
        if (img) {
            cover->setPixmap(QPixmap::fromImage(*img));
        }
    }
}
