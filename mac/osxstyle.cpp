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

#include "mac/osxstyle.h"
#include "core/globalstatic.h"
#include "ui/actioncollection.h"
#include "ui/action.h"
#include "ui/utils.h"
#include <QApplication>
#include <QStyle>
#include <QTreeWidget>
#include <QPainter>
#include <QMenu>
#include <QMenuBar>
#include <QWidget>
#include <QMainWindow>
#include <qnamespace.h>

GLOBAL_STATIC(Mac::OSXStyle, instance)

Mac::OSXStyle::OSXStyle()
    : view(0)
    , windowMenu(0)
    , closeAct(0)
    , minAct(0)
    , zoomAct(0)
{
}

const QPalette & Mac::OSXStyle::viewPalette() {
    return viewWidget()->palette();
}

void Mac::OSXStyle::drawSelection(QStyleOptionViewItemV4 opt, QPainter *painter, double opacity) {
    opt.palette=viewPalette();
    if (opacity<0.999) {
        QColor col(opt.palette.highlight().color());
        col.setAlphaF(opacity);
        opt.palette.setColor(opt.palette.currentColorGroup(), QPalette::Highlight, col);
    }
    QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, viewWidget());
}

QColor Mac::OSXStyle::monoIconColor() {
    return QColor(96, 96, 96);
}

void Mac::OSXStyle::initWindowMenu(QMainWindow *mw) {
    if (!windowMenu && mw) {
        windowMenu=new QMenu(tr("&Window"), mw);
        closeAct=Ui::ActionCollection::get()->createAction("close-window", tr("Close"));
        minAct=Ui::ActionCollection::get()->createAction("minimize-window", tr("Minimize"));
        zoomAct=Ui::ActionCollection::get()->createAction("zoom-window", tr("Zoom"));
        windowMenu->addAction(closeAct);
        windowMenu->addAction(minAct);
        windowMenu->addAction(zoomAct);
        windowMenu->addSeparator();
        addWindow(mw);
        mw->menuBar()->addMenu(windowMenu);
        actions[mw]->setChecked(true);
        connect(qApp, SIGNAL(focusWindowChanged(QWindow *)), SLOT(focusWindowChanged(QWindow *)));
        closeAct->setShortcut(Qt::ControlModifier+Qt::Key_W);
        minAct->setShortcut(Qt::ControlModifier+Qt::Key_M);
        connect(closeAct, SIGNAL(triggered()), SLOT(closeWindow()));
        connect(minAct, SIGNAL(triggered()), SLOT(minimizeWindow()));
        connect(zoomAct, SIGNAL(triggered()), SLOT(zoomWindow()));
        controlActions(mw);
    }
}

void Mac::OSXStyle::addWindow(QWidget *w) {
    if (w && windowMenu && !actions.contains(w)) {
        QAction *action=windowMenu->addAction(w->windowTitle());
        action->setCheckable(true);
        connect(action, SIGNAL(triggered()), this, SLOT(showWindow()));
        connect(w, SIGNAL(windowTitleChanged(QString)), this, SLOT(windowTitleChanged()));
        actions.insert(w, action);
    }
}

void Mac::OSXStyle::removeWindow(QWidget *w) {
    if (w && windowMenu && actions.contains(w)) {
        QAction *act=actions.take(w);
        windowMenu->removeAction(act);
        disconnect(act, SIGNAL(triggered()), this, SLOT(showWindow()));
        disconnect(w, SIGNAL(windowTitleChanged(QString)), this, SLOT(windowTitleChanged()));
        act->deleteLater();
    }
}

void Mac::OSXStyle::showWindow() {
    QAction *act=qobject_cast<QAction *>(sender());

    if (!act) {
        return;
    }

    QMap<QWidget *, QAction *>::Iterator it=actions.begin();
    QMap<QWidget *, QAction *>::Iterator end=actions.end();

    for (; it!=end; ++it) {
        if (it.value()==act) {
            Ui::Utils::raiseWindow(it.key());
        }
        act->setChecked(it.value()==act);
    }
}

void Mac::OSXStyle::windowTitleChanged() {
    QWidget *w=qobject_cast<QWidget *>(sender());

    if (!w) {
        return;
    }
    if (actions.contains(w)) {
        actions[w]->setText(w->windowTitle());
    }
}

void Mac::OSXStyle::focusWindowChanged(QWindow *win) {
    QMap<QWidget *, QAction *>::Iterator it=actions.begin();
    QMap<QWidget *, QAction *>::Iterator end=actions.end();

    for (; it!=end; ++it) {
        if (it.key()->windowHandle()==win) {
            it.value()->setChecked(true);
            controlActions(it.key());
        } else {
            it.value()->setChecked(false);
        }
    }
}

void Mac::OSXStyle::closeWindow() {
    QWidget *w=currentWindow();
    if (w) {
        w->close();
    }
}

void Mac::OSXStyle::minimizeWindow() {
    QWidget *w=currentWindow();
    if (w) {
        w->showMinimized();
    }
}

void Mac::OSXStyle::zoomWindow() {
    QWidget *w=currentWindow();
    if (w) {
        if (w->isMaximized()) {
            w->showNormal();
        } else {
            w->showMaximized();
        }
    }
}

QWidget * Mac::OSXStyle::currentWindow() {
    QMap<QWidget *, QAction *>::Iterator it=actions.begin();
    QMap<QWidget *, QAction *>::Iterator end=actions.end();

    for (; it!=end; ++it) {
        if (it.value()->isChecked()) {
            return it.key();
        }
    }
    return 0;
}

void Mac::OSXStyle::controlActions(QWidget *w) {
    closeAct->setEnabled(w && w->windowFlags()&Qt::WindowCloseButtonHint);
    minAct->setEnabled(w && w->windowFlags()&Qt::WindowMinimizeButtonHint);
    zoomAct->setEnabled(w && w->minimumHeight()!=w->maximumHeight());
}

QTreeWidget * Mac::OSXStyle::viewWidget() {
    if (!view) {
        view=new QTreeWidget();
        view->ensurePolished();
    }
    return view;
}
