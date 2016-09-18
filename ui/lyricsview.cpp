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

#include "ui/lyricsview.h"
#include "ui/viewtoolbar.h"
#include "ui/proxystyle.h"
#include "core/lyrics.h"
#include <QVBoxLayout>
#include <QTextEdit>
#include <QLabel>
#include <QScroller>

Ui::LyricsView::LyricsView(QWidget *p)
    : QWidget(p)
{
    QVBoxLayout *lay=new QVBoxLayout(this);
    ViewToolBar *toolbar=new ViewToolBar(this);
    lyrics=new Core::Lyrics(this);
    text=new QTextEdit(this);
    lay->addWidget(toolbar);
    lay->addWidget(text);
    lay->setMargin(0);
    lay->setSpacing(0);
    text->setProperty(Ui::ProxyStyle::constModifyFrameProp, ProxyStyle::VF_Side);
    text->setReadOnly(true);
    text->setText(tr("No lyrics"));
    connect(lyrics, SIGNAL(fetched(QString,QString,QString)), this, SLOT(fetched(QString,QString,QString)));
    toolbar->setTitle(tr("Lyrics"));
    QScroller::grabGesture(text->viewport());
}

void Ui::LyricsView::showEvent(QShowEvent *ev) {
    QWidget::showEvent(ev);
    lyrics->setEnabled(true);
}

void Ui::LyricsView::hideEvent(QHideEvent *ev) {
    QWidget::hideEvent(ev);
    lyrics->setEnabled(false);
}

void Ui::LyricsView::fetched(const QString &artist, const QString &title, const QString &contents) {
    text->setText(contents);
}
