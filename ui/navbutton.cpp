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

#include "ui/navbutton.h"
#include "ui/utils.h"
#include "core/utils.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMenu>

static const int constSpace=8;

Ui::NavButton::ProxyStyle::ProxyStyle()
    : QProxyStyle()
{
    setBaseStyle(qApp->style());
}

void Ui::NavButton::ProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const {
    if (CC_ToolButton==control) {
        if (const QStyleOptionToolButton *tb = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            int iconWidth = tb->icon.isNull() ? 0 : tb->iconSize.isValid() ? tb->iconSize.width() : baseStyle()->pixelMetric(PM_ToolBarIconSize, option, widget);
            int frameWidth=baseStyle()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            QRect textRect(tb->rect);
            bool ltr=Qt::LeftToRight==qApp->layoutDirection();
            int space=Utils::scaleForDpi(constSpace);
            int menuSpace=tb->features&QStyleOptionToolButton::HasMenu ? space*2 : 0;
            if (ltr) {
                textRect.adjust(iconWidth+space+frameWidth, 0, -(frameWidth+menuSpace), 0);
            } else {
                textRect.adjust(frameWidth+menuSpace, 0, -(iconWidth+space+frameWidth), 0);
            }
            QFontMetrics fm(painter->fontMetrics());
            QString text=fm.elidedText(tb->text, ltr ? Qt::ElideLeft : Qt::ElideRight, textRect.width(), Qt::TextShowMnemonic);

            if (option->state&State_Sunken || option->state&State_MouseOver) {
                QStyleOptionToolButton copy=*tb;
                copy.text=QString();
                copy.icon=QIcon();
                baseStyle()->drawComplexControl(control, &copy, painter, widget);
            }

            QPixmap pix=tb->icon.pixmap(tb->iconSize);
            painter->setPen(option->palette.windowText().color());
            if (0==textRect.height()%2) {
                textRect.setHeight(textRect.height()-1);
            }
            painter->drawText(textRect, Qt::TextHideMnemonic|Qt::AlignVCenter|(ltr ? Qt::AlignLeft : Qt::AlignRight), text);
            if (ltr) {
                painter->drawPixmap(frameWidth+(space/2), (tb->rect.height()-pix.height())/2, pix);
            } else {
                painter->drawPixmap(tb->rect.width()-frameWidth+(space/2)+iconWidth, (tb->rect.height()-pix.height())/2, pix);
            }
            return;
        }
    }
    baseStyle()->drawComplexControl(control, option, painter, widget);
}

Ui::NavButton::ProxyStyle *Ui::NavButton::proxyStyle=0;

static const char *constIdxProperty="idx";
static const char *constIdProperty="id";
static const char *constPathProperty="path";

static QString actPath(QAction *act) {
    if (!act->property(constPathProperty).toString().isEmpty()) {
        return act->property(constPathProperty).toString();
    } else {
        return Core::Utils::strippedText(act->text());
    }
}

Ui::NavButton::NavButton(QWidget *p)
    : Ui::ToolButton(p)
{
    if (!proxyStyle) {
        proxyStyle=new ProxyStyle();
    }
    setStyle(proxyStyle);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

QAction * Ui::NavButton::add(QString str, int id, const QIcon &icon) {
    str.replace("&", "&&");
    QAction *act=menu()->addAction(str);
    act->setProperty(constIdProperty, id);
    act->setIcon(icon);
    setText(str);
    setIcon(icon);
    return act;
}

QAction * Ui::NavButton::add(QString str, const QModelIndex &idx, const QIcon &icon) {
    str.replace("&", "&&");
    QAction *act=menu()->addAction(str);
    act->setProperty(constIdxProperty, idx);
    act->setIcon(icon);
    setText(str);
    setIcon(icon);
    return act;
}

QAction *Ui::NavButton::add(const QModelIndex &idx, const QIcon &icon) {
    QAction *act=add(idx.data().toString(), idx, icon);
    QString path=idx.data().toString();
    QModelIndex i=idx.parent();
    QString sep=QLatin1String("  ")+QChar(Qt::LeftToRight==layoutDirection() ? 0x25B8 : 0x25C2)+QLatin1String("  ");

    while (i.isValid()) {
        path=i.data().toString()+sep+path;
        i=i.parent();
    }
    if (idx.model()) {
        QString modelName=idx.model()->data(QModelIndex()).toString();
        if (!modelName.isEmpty()) {
            path=modelName+sep+path;
        }
    }
    path.replace("&", "&&");
    act->setProperty(constPathProperty, path);
    setText(path);
    return act;
}

void Ui::NavButton::remove(const QModelIndex &idx) {
    for (int i=0; i<menu()->actions().count(); ++i) {
        if (menu()->actions().at(i)->property(constIdxProperty).toModelIndex()==idx) {
            menu()->actions().removeAt(i);
            break;
        }
    }
    if (!menu()->actions().isEmpty()) {
        QAction *act=menu()->actions().last();
        setText(actPath(act));
        setIcon(act->icon());
    } else {
        setText(QString());
        setIcon(QIcon());
    }
}

void Ui::NavButton::removeFrom(const QModelIndex &idx) {
    QList<QAction *> toRemove;
    QIcon icon;
    QString newText;
    bool add=false;
    foreach (QAction *act, menu()->actions()) {
        if (add) {
            toRemove.append(act);
        } else if (act->property(constIdxProperty).isValid() && act->property(constIdxProperty).toModelIndex()==idx) {
            newText=actPath(act);
            icon=act->icon();
            add=true;
        }
    }
    foreach (QAction *act, toRemove) {
        menu()->actions().removeAll(act);
        act->deleteLater();
    }
    setText(newText);
    setIcon(icon);
}

void Ui::NavButton::clear() {
    QList<QAction *> toRemove;
    QString newText;
    QIcon icon;
    foreach (QAction *act, menu()->actions()) {
        if (act->property(constIdxProperty).isValid()) {
            toRemove.append(act);
        } else {
            newText=actPath(act);
            icon=act->icon();
        }
    }
    foreach (QAction *act, toRemove) {
        menu()->actions().removeAll(act);
        act->deleteLater();
    }
    setText(newText);
    setIcon(icon);
}

QSize Ui::NavButton::sizeHint() const {
    return ToolButton::sizeHint()+QSize(Utils::scaleForDpi(QToolButton::menu() ? (constSpace*2) : (constSpace/2)), Utils::scaleForDpi(constSpace));
}

void Ui::NavButton::itemSelected(QAction *act) {
    if (act->property(constIdxProperty).isValid()) {
        emit selected(act->property(constIdxProperty).toModelIndex());
    } else {
        emit selected(act->property(constIdProperty).toInt());
    }
}

QMenu * Ui::NavButton::menu() {
    if (!QToolButton::menu()) {
        QMenu *m=new QMenu(this);
        setMenu(m);
        connect(m, SIGNAL(triggered(QAction*)), SLOT(itemSelected(QAction*)));
        setPopupMode(QToolButton::DelayedPopup);
        setHideMenuIndicator(false);
    }
    return QToolButton::menu();
}
