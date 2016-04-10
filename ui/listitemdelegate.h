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

#ifndef UI_LIST_ITEM_DELEGATE
#define UI_LIST_ITEM_DELEGATE

#include "ui/basicitemdelegate.h"

namespace Ui {
class ListView;

class ListItemDelegate : public BasicItemDelegate
{
    Q_OBJECT
public:
    static void getSizes(int &border, int &listCover);
    static void drawPlayState(QPainter *painter, const QStyleOptionViewItem &option, const QRect &r, int state);

    ListItemDelegate(ListView *v);
    virtual ~ListItemDelegate();

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const;
    int getAction(const QModelIndex &index) const;

public Q_SLOTS:
    bool helpEvent(QHelpEvent *e, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index);

private:
    void drawIcons(QPainter *painter, const QRect &r, bool mouseOver, bool rtl, bool onTop, const QModelIndex &index) const;
    QRect calcActionRect(bool rtl, bool onTop, const QRect &rect) const;
    static void adjustActionRect(bool rtl, bool onTop, QRect &rect);

private:
    ListView *view;
};

}

#endif
