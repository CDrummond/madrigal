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

#ifndef UI_VIEWTOOLBAR_H
#define UI_VIEWTOOLBAR_H

#include <QStackedWidget>

class QHBoxLayout;
class QLabel;
namespace Ui {
class SqueezedTextLabel;
class ViewToolBar : public QStackedWidget {
public:
    static int iconSize();

    ViewToolBar(QWidget *p);
    void setTitle(const QString &str);
    void addWidget(QWidget *w, bool left=true);
    void addSpacer(int space, bool left=true);
    void showEvent(QShowEvent *ev);
    void paintEvent(QPaintEvent *ev);
    void showTitle(bool s) { setCurrentIndex(s ? 0 : 1); }
private:
    QHBoxLayout *layout;
    QLabel *title;
};
}

#endif
