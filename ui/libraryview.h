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

#ifndef UI_LIBRARY_VIEW_H
#define UI_LIBRARY_VIEW_H

#include "ui/listview.h"
#include "core/actions.h"
namespace Ui {
class LibraryView : public ListView {
    Q_OBJECT
public:
    LibraryView(QWidget *p);
    virtual ~LibraryView() { }

    void setIconMode(bool i);
    void setModel(QAbstractItemModel *m);
    void setRootIndex(const QModelIndex &index);

Q_SIGNALS:
    void doAction(int act);
    void itemClicked(const QModelIndex &index);

private Q_SLOTS:
    void checkItemClicked(const QModelIndex &index);
    void checkItemActivated(const QModelIndex &index);

private:
    Core::Actions::Type getAction(const QModelIndex &index);

private:
    QList<int> prevRows;
};
}

#endif
