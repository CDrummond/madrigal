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

#ifndef UI_NAVBUTTON_H
#define UI_NAVBUTTON_H

#include "ui/flattoolbutton.h"
#include <QProxyStyle>

class QModelIndex;

namespace Ui {

class NavButton : public FlatToolButton {
    Q_OBJECT

private:
    class ProxyStyle : public QProxyStyle {
    public:
        ProxyStyle();
        void drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const;
    };

    static ProxyStyle *proxyStyle;

public:
    NavButton(QWidget *p);
    ~NavButton() { }

    QAction * add(QString str, int id, const QIcon &icon=QIcon());
    QAction * add(QString str, const QModelIndex &idx, const QIcon &icon=QIcon());
    QAction * add(const QModelIndex &idx, const QIcon &icon=QIcon());
    void remove(const QModelIndex &idx);
    void removeFrom(const QModelIndex &idx);
    void clear();
    QSize sizeHint() const;

Q_SIGNALS:
    void selected(const QModelIndex &idx);
    void selected(int id);

private Q_SLOTS:
    void itemSelected(QAction *act);

private:
    QMenu * menu();
};

}

#endif
