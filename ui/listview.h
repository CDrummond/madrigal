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

#ifndef UI_LISTVIEW_H
#define UI_LISTVIEW_H

#include <QListView>
#include "core/images.h"

namespace Ui {
class ListView : public QListView {
    Q_OBJECT

public:
    ListView(QWidget *parent=0);
    virtual ~ListView();

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    bool haveSelectedItems() const;
    bool haveUnSelectedItems() const;
    void mouseReleaseEvent(QMouseEvent *event);
    QModelIndexList selectedIndexes() const { return selectedIndexes(true); }
    QModelIndexList selectedIndexes(bool sorted) const;
    virtual void setModel(QAbstractItemModel *m);

public Q_SLOTS:
    void coverFound(const Core::ImageDetails &cover);

private Q_SLOTS:
    void correctSelection();

Q_SIGNALS:
    bool itemsSelected(bool);
};

}

#endif
