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

#include "ui/listview.h"
#include "upnp/device.h"
#include <QMimeData>
#include <QDrag>
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QPaintEvent>
#include <QModelIndex>

Ui::ListView::ListView(QWidget *parent)
    : QListView(parent)
{
    setDragEnabled(true);
    setContextMenuPolicy(Qt::NoContextMenu);
    setDragDropMode(QAbstractItemView::DragOnly);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setAlternatingRowColors(false);
    setUniformItemSizes(true);
    setAttribute(Qt::WA_MouseTracking);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    #ifdef Q_OS_MAC
    setAttribute(Qt::WA_MacShowFocusRect, 0);
    #endif
    setWordWrap(false);
}

Ui::ListView::~ListView() {
}

void Ui::ListView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QListView::selectionChanged(selected, deselected);
    bool haveSelection=haveSelectedItems();

//    setContextMenuPolicy(haveSelection ? Qt::ActionsContextMenu : (menu ? Qt::CustomContextMenu : Qt::NoContextMenu));
    emit itemsSelected(haveSelection);
}

bool Ui::ListView::haveSelectedItems() const {
    // Dont need the sorted type of 'selectedIndexes' here...
    return selectionModel() && selectionModel()->selectedIndexes().count()>0;
}

bool Ui::ListView::haveUnSelectedItems() const {
    // Dont need the sorted type of 'selectedIndexes' here...
    return selectionModel() && selectionModel()->selectedIndexes().count()!=model()->rowCount();
}

void Ui::ListView::mouseReleaseEvent(QMouseEvent *event) {
    if (Qt::NoModifier==event->modifiers() && Qt::LeftButton==event->button()) {
        QListView::mouseReleaseEvent(event);
    }
}

QModelIndexList Ui::ListView::selectedIndexes(bool sorted) const {
    QModelIndexList indexes=selectionModel() ? selectionModel()->selectedIndexes() : QModelIndexList();
    if (sorted) {
        qSort(indexes);
    }
    return indexes;
}

void Ui::ListView::setModel(QAbstractItemModel *m) {
    QAbstractItemModel *old=model();
    QListView::setModel(m);

    if (old) {
        disconnect(old, SIGNAL(layoutChanged()), this, SLOT(correctSelection()));
    }

    if (m && old!=m) {
        connect(m, SIGNAL(layoutChanged()), this, SLOT(correctSelection()));
    }
}

void Ui::ListView::coverFound(const Core::ImageDetails &cover) {
    if (model()) {
        int count=model()->rowCount(rootIndex());
        for (int i=0; i<count; ++i) {
            QModelIndex idx=model()->index(i, 0, rootIndex());
            if (idx.internalPointer()) {
                Core::ImageDetails cvr=static_cast<Upnp::Device::Item *>(idx.internalPointer())->cover();
                if (cover.isBroadcast==cvr.isBroadcast && cvr.album==cover.album && cvr.artist==cover.artist) {
                    update(idx);
                }
            }
        }
    }
}

// Workaround for https://bugreports.qt-project.org/browse/QTBUG-18009
void Ui::ListView::correctSelection() {
    if (!selectionModel()) {
        return;
    }

    QItemSelection s = selectionModel()->selection();
    setCurrentIndex(currentIndex());
    selectionModel()->select(s, QItemSelectionModel::SelectCurrent);
}
