/*
 * Madrigal
 *
 * Copyright (c) 2016 Craig Drummond <craig.p.drummond@gmail.com>
 *
 */

// Copied from oxygenwindowmanager.cpp svnversion: 1139230

//////////////////////////////////////////////////////////////////////////////
// oxygenwindowmanager.cpp
// pass some window mouse press/release/move event actions to window manager
// -------------------
//
// Copyright (c) 2010 Hugo Pereira Da Costa <hugo@oxygen-icons.org>
//
// Largely inspired from BeSpin style
// Copyright (C) 2007 Thomas Luebking <thomas.luebking@web.de>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//////////////////////////////////////////////////////////////////////////////

#include "windowmanager.h"
#include <QApplication>
#include <QDialog>
#include <QComboBox>
#include <QDockWidget>
#include <QLabel>
#include <QMainWindow>
#include <QMenuBar>
#include <QMouseEvent>
#include <QStyle>
#include <QToolBar>
#include <QToolButton>
#include <QProgressBar>

static inline bool isToolBar(QWidget *w) {
    return qobject_cast<QToolBar*>(w) || 0==strcmp(w->metaObject()->className(), "ToolBar");
}

static inline void addEventFilter(QObject *object, QObject *filter) {
    object->removeEventFilter(filter);
    object->installEventFilter(filter);
}

Mac::WindowManager::WindowManager(QObject *parent)
    : QObject(parent)
    , _dragDistance(QApplication::startDragDistance())
    , _dragDelay(QApplication::startDragTime())
    , _dragAboutToStart(false)
    , _dragInProgress(false)
    , _locked(false)
    #ifndef Q_OS_MAC
    , _cursorOverride(false)
    #endif
{
    // install application wise event filter
    _appEventFilter = new AppEventFilter(this);
    qApp->installEventFilter(_appEventFilter);
}

void Mac::WindowManager::registerWidgetAndChildren(QWidget *w) {
    QObjectList children=w->children();

    foreach (QObject *o, children) {
        if (qobject_cast<QWidget *>(o)) {
            registerWidgetAndChildren((QWidget *)o);
        }
    }
    registerWidget(w);
}

void Mac::WindowManager::registerWidget(QWidget *widget) {
    if (isDragable(widget)) {
        addEventFilter(widget, this);
    }
}

void Mac::WindowManager::unregisterWidget(QWidget *widget) {
    if (widget) {
        widget->removeEventFilter(this);
    }
}

bool Mac::WindowManager::eventFilter(QObject *object, QEvent *event) {
    switch (event->type()) {
    case QEvent::MouseButtonPress:
        return mousePressEvent(object, event);
        break;
    case QEvent::MouseMove:
        if (object == _target.data()) {
            return mouseMoveEvent(object, event);
        }
        break;
    case QEvent::MouseButtonRelease:
        if (_target) {
            return mouseReleaseEvent(object, event);
        }
        break;
    default:
        break;
    }
    return false;
}

void Mac::WindowManager::timerEvent(QTimerEvent *event) {
    if (event->timerId() == _dragTimer.timerId()) {
        _dragTimer.stop();
        if (_target) {
            startDrag(_target.data(), _globalDragPoint);
        }
    } else {
        return QObject::timerEvent(event);
    }
}

bool Mac::WindowManager::mousePressEvent(QObject *object, QEvent *event) {
    // cast event and check buttons/modifiers
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    if (!(Qt::NoModifier==mouseEvent->modifiers() && Qt::LeftButton==mouseEvent->button())) {
        return false;
    }

    // check lock
    if (isLocked()) {
        return false;
    } else {
        setLocked(true);
    }

    // cast to widget
    QWidget *widget = static_cast<QWidget*>(object);

    // check if widget can be dragged from current position
    if (!canDrag(widget)) {
        return false;
    }

    // retrieve widget's child at event position
    QPoint position(mouseEvent->pos());
    QWidget *child = widget->childAt(position);
    if (!canDrag(widget, child, position)) {
        return false;
    }

    // save target and drag point
    _target = widget;
    _dragPoint = position;
    _globalDragPoint = mouseEvent->globalPos();
    _dragAboutToStart = true;

    // send a move event to the current child with same position
    // if received, it is caught to actually start the drag
    QPoint localPoint(_dragPoint);
    if (child) {
        localPoint = child->mapFrom(widget, localPoint);
    } else {
        child = widget;
    }
    QMouseEvent localMouseEvent(QEvent::MouseMove, localPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(child, &localMouseEvent);
    // never eat event
    return false;
}

bool Mac::WindowManager::mouseMoveEvent(QObject *object, QEvent *event) {
    Q_UNUSED(object)

    // stop timer
    if (_dragTimer.isActive()){
        _dragTimer.stop();
    }

    // cast event and check drag distance
    QMouseEvent *mouseEvent = static_cast<QMouseEvent*>(event);
    if (!_dragInProgress) {
        if (_dragAboutToStart) {
            if (mouseEvent->globalPos() == _globalDragPoint) {
                // start timer,
                _dragAboutToStart = false;
                if (_dragTimer.isActive()) {
                    _dragTimer.stop();
                }
                _dragTimer.start(_dragDelay, this);

            } else {
                resetDrag();
            }
        } else if (QPoint(mouseEvent->globalPos() - _globalDragPoint).manhattanLength() >= _dragDistance) {
            _dragTimer.start(0, this);
        }
        return true;
    } else {
        // use QWidget::move for the grabbing
        /* this works only if the sending object and the target are identical */
        QWidget *window(_target.data()->window());
        window->move(window->pos() + mouseEvent->pos() - _dragPoint);
        return true;
    }
}

bool Mac::WindowManager::mouseReleaseEvent(QObject *object, QEvent *event) {
    Q_UNUSED(object)
    Q_UNUSED(event)
    resetDrag();
    return false;
}

bool Mac::WindowManager::isDragable(QWidget *widget) {
    // check widget
    if (!widget) {
        return false;
    }

    // accepted default types
    if (qobject_cast<QDialog*>(widget) && widget->isWindow() || qobject_cast<QMainWindow*>(widget) && widget->isWindow()) {
        return true;
    }

    // more accepted types, provided they are not dock widget titles
    if ((qobject_cast<QMenuBar*>(widget) || isToolBar(widget)) && !isDockWidgetTitle(widget)) {
        return true;
    }

    // flat toolbuttons
    if (QToolButton *toolButton = qobject_cast<QToolButton*>(widget)) {
        if (toolButton->autoRaise()) {
            return true;
        }
    }

    return false;
}

bool Mac::WindowManager::canDrag(QWidget *widget) {
    // assume isDragable widget is already passed
    // check some special cases where drag should not be effective

    // check mouse grabber
    if (QWidget::mouseGrabber()) {
        return false;
    }

    /*
        check cursor shape.
        Assume that a changed cursor means that some action is in progress
        and should prevent the drag
        */
    if (Qt::ArrowCursor!=widget->cursor().shape()) {
        return false;
    }

    // accept
    return true;
}

bool Mac::WindowManager::canDrag(QWidget *widget, QWidget *child, const QPoint &position) {
    // retrieve child at given position and check cursor again
    if (child && Qt::ArrowCursor!=child->cursor().shape()) {
        return false;
    }

    /*
        check against children from which drag should never be enabled,
        even if mousePress/Move has been passed to the parent
        */
    if (child && (qobject_cast<QComboBox*>(child) || qobject_cast<QProgressBar*>(child))) {
        return false;
    }

    // tool buttons
    if (QToolButton *toolButton = qobject_cast<QToolButton*>(widget)) {
        if (!isToolBar(widget->parentWidget())) {
            return false;
        }
        return toolButton->autoRaise() && !toolButton->isEnabled();
    }

    // check menubar
    if (QMenuBar *menuBar = qobject_cast<QMenuBar*>(widget)) {
        // check if there is an active action
        if (menuBar->activeAction() && menuBar->activeAction()->isEnabled()) {
            return false;
        }

        // check if action at position exists and is enabled
        if (QAction *action = menuBar->actionAt(position)) {
            if (action->isSeparator()) {
                return true;
            }
            if (action->isEnabled()) {
                return false;
            }
        }

        // return true in all other cases
        return true;
    }

    return isToolBar(widget);
}

void Mac::WindowManager::resetDrag() {
    #ifndef Q_OS_MAC
    if (_target && _cursorOverride) {
        qApp->restoreOverrideCursor();
        _cursorOverride = false;
    }
    #endif

    _target.clear();
    if (_dragTimer.isActive()) {
        _dragTimer.stop();

    }
    _dragPoint = QPoint();
    _globalDragPoint = QPoint();
    _dragAboutToStart = false;
    _dragInProgress = false;
}

void Mac::WindowManager::startDrag(QWidget *widget, const QPoint &position) {
    if (QWidget::mouseGrabber()) {
        return;
    }

    #ifndef Q_OS_MAC
    if (!_cursorOverride) {
        qApp->setOverrideCursor(Qt::DragMoveCursor);
        _cursorOverride = true;
    }
    #endif

    _dragInProgress = true;
    return;
}

bool Mac::WindowManager::AppEventFilter::eventFilter(QObject *object, QEvent *event) {
    Q_UNUSED(object)
    if (QEvent::MouseButtonRelease==event->type()) {
        // stop drag timer
        if (_parent->_dragTimer.isActive()) {
            _parent->resetDrag();
        }

        // unlock
        if (_parent->isLocked()) {
            _parent->setLocked(false);
        }
    }

    return false;
}

bool Mac::WindowManager::isDockWidgetTitle(const QWidget *widget) const {
    if (!widget) {
        return false;
    }
    if (const QDockWidget *dockWidget = qobject_cast<const QDockWidget*>(widget->parent())) {
        return widget == dockWidget->titleBarWidget();
    } else {
       return false;
    }
}

bool Mac::WindowManager::AppEventFilter::appMouseEvent(QObject *object, QEvent *event) {
    Q_UNUSED(object)

    // store target window (see later)
    QWidget *window(_parent->_target.data()->window());

    /*
        post some mouseRelease event to the target, in order to counter balance
        the mouse press that triggered the drag. Note that it triggers a resetDrag
        */
    QMouseEvent mouseEvent(QEvent::MouseButtonRelease, _parent->_dragPoint, Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    qApp->sendEvent(_parent->_target.data(), &mouseEvent);

    if (QEvent::MouseMove==event->type()) {
        /*
            HACK: quickly move the main cursor out of the window and back
            this is needed to get the focus right for the window children
            the origin of this issue is unknown at the moment
            */
        const QPoint cursor = QCursor::pos();
        QCursor::setPos(window->mapToGlobal(window->rect().topRight()) + QPoint(1, 0));
        QCursor::setPos(cursor);

    }
    return true;
}
