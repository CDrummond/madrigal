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

#include "ui/menubutton.h"
#include <QAction>
#include <QMenu>
#include <QEvent>
#include <QApplication>
#include <QDesktopWidget>

Ui::MenuButton::MenuButton(QWidget *parent)
    : ToolButton(parent)
{
    setPopupMode(QToolButton::InstantPopup);
    setToolTip(tr("Menu"));
    setHideMenuIndicator(true);
    installEventFilter(this);
}

void Ui::MenuButton::controlState() {
    if (!menu()) {
        return;
    }
    foreach (QAction *a, menu()->actions()) {
        if (a->isEnabled() && a->isVisible() && !a->isSeparator()) {
            setEnabled(true);
            return;
        }
    }
    setEnabled(false);
}

void Ui::MenuButton::setAlignedMenu(QMenu *m) {
    QToolButton::setMenu(m);
    m->installEventFilter(this);
}

void Ui::MenuButton::addSeparator() {
    QAction *sep=new QAction(this);
    sep->setSeparator(true);
    addAction(sep);
}

bool Ui::MenuButton::eventFilter(QObject *o, QEvent *e) {
    if (QEvent::Show==e->type()) {
        if (qobject_cast<QMenu *>(o)) {
            QMenu *mnu=static_cast<QMenu *>(o);
            QPoint p=parentWidget()->mapToGlobal(pos());
            int newPos=isRightToLeft()
                    ? p.x()
                    : ((p.x()+width())-mnu->width());

            if (newPos<0) {
                newPos=0;
            } else {
                QDesktopWidget *dw=QApplication::desktop();
                if (dw) {
                    QRect geo=dw->availableGeometry(this);
                    int maxWidth=geo.x()+geo.width();
                    if (maxWidth>0 && (newPos+mnu->width())>maxWidth) {
                        newPos=maxWidth-mnu->width();
                    }
                }
            }
            mnu->move(newPos, mnu->y());
        } else if (o==this) {
            setMinimumWidth(height());
            removeEventFilter(this);
        }
    }

    return ToolButton::eventFilter(o, e);
}
