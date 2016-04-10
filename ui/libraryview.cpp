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

#include "ui/libraryview.h"
#include "ui/listitemdelegate.h"

Ui::LibraryView::LibraryView(QWidget *p)
    : ListView(p)
{
    setItemDelegate(new ListItemDelegate(this));
    connect(this, SIGNAL(clicked(const QModelIndex &)), this, SLOT(checkItemClicked(const QModelIndex &)));
    connect(this, SIGNAL(activated(QModelIndex)), this, SIGNAL(itemClicked(QModelIndex)));
}

void Ui::LibraryView::setIconMode(bool i) {
    setViewMode(i ? QListView::IconMode : QListView::ListMode);
    setResizeMode(i ? QListView::Adjust : QListView::Fixed);
    setGridSize(i ? itemDelegate()->sizeHint(QStyleOptionViewItem(), QModelIndex()) : QSize(-1, -1));
}

void Ui::LibraryView::setModel(QAbstractItemModel *m) {
    ListView::setModel(m);
    prevRows.clear();
}

void Ui::LibraryView::setRootIndex(const QModelIndex &index) {
    QModelIndex prevTop=indexAt(QPoint(8, 8));
    QModelIndex prevRoot=rootIndex();
    ListView::setRootIndex(index);
    if (index.isValid() && index.parent()==prevRoot) {
        // Gone 1 level down tree
        prevRows.append(prevTop.row());
    } else if (!prevRows.isEmpty()) {
        QModelIndex idx=model()->index(prevRows.last(), 0, index);
        // Gone 1 level up tree
        scrollTo(model()->index(prevRows.takeLast(), 0, index), QAbstractItemView::PositionAtTop);
    }
}

void Ui::LibraryView::checkItemClicked(const QModelIndex &index) {
    Core::Actions::Type act=getAction(index);
    if (Core::Actions::Action_None!=act) {
        emit doAction(act);
        return;
    }

    emit itemClicked(index);
}

Core::Actions::Type Ui::LibraryView::getAction(const QModelIndex &index) {
    QAbstractItemDelegate *abs=itemDelegate();
    ListItemDelegate *d=abs ? qobject_cast<ListItemDelegate *>(abs) : 0;
    return d ? (Core::Actions::Type)d->getAction(index) : Core::Actions::Action_None;
}

QModelIndex Ui::LibraryView::createIndex() {
    QModelIndex idx;
    if (!prevRows.isEmpty()) {
        for (int i=prevRows.count()-1; i>=0 ; --i) {
            int row=prevRows.at(i);
            if (-1==row) {
                break;
            }
            idx=model()->index(row, 0, idx);
            if (!idx.isValid()) {
                break;
            }
        }
    }
    return idx;
}