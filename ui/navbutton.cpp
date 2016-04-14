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
#include "core/utils.h"
#include <QApplication>
#include <QStyle>
#include <QStyleOption>
#include <QPainter>
#include <QMenu>

Ui::NavButton::ProxyStyle::ProxyStyle()
    : QProxyStyle()
{
    setBaseStyle(qApp->style());
}

void Ui::NavButton::ProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const {
    if (CC_ToolButton==control) {
        if (const QStyleOptionToolButton *toolbutton = qstyleoption_cast<const QStyleOptionToolButton *>(option)) {
            QStyleOptionToolButton copy=*toolbutton;
            int iconWidth = copy.icon.isNull() ? 0 : copy.iconSize.isValid() ? copy.iconSize.width() : baseStyle()->pixelMetric(PM_ToolBarIconSize, option, widget);
            int frameWidth=baseStyle()->pixelMetric(PM_DefaultFrameWidth, option, widget);
            QRect textRect(toolbutton->rect);
            bool ltr=Qt::LeftToRight==qApp->layoutDirection();
            if (ltr) {
                textRect.adjust(iconWidth+8+frameWidth, frameWidth, -(frameWidth+4), -frameWidth);
            } else {
                textRect.adjust(frameWidth+4, frameWidth, -(iconWidth+8+frameWidth), -frameWidth);
            }
            QFontMetrics fm(painter->fontMetrics());
            QString text=fm.elidedText(toolbutton->text, ltr ? Qt::ElideLeft : Qt::ElideRight, textRect.width());
            copy.text=QString();
            copy.icon=QIcon();
            if (option->state&State_Sunken || option->state&State_MouseOver) {
                baseStyle()->drawComplexControl(control, &copy, painter, widget);
            } else if (!copy.icon.isNull()) {
                copy.features=0;
                baseStyle()->drawComplexControl(control, &copy, painter, widget);
            }
            QTextOption textOpt(Qt::AlignVCenter|(ltr ? Qt::AlignLeft : Qt::AlignRight));
            QPixmap pix=toolbutton->icon.pixmap(toolbutton->iconSize);
            painter->setPen(option->palette.windowText().color());
            painter->drawText(textRect, text, textOpt);
            if (ltr) {
                painter->drawPixmap(frameWidth+4, (toolbutton->rect.height()-iconWidth)/2, pix);
            } else {
                painter->drawPixmap(toolbutton->rect.width()-frameWidth+4+iconWidth, (toolbutton->rect.height()-iconWidth)/2, pix);
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
    : Ui::FlatToolButton(p)
{
    if (!proxyStyle) {
        proxyStyle=new ProxyStyle();
    }
    setStyle(proxyStyle);
    setMenu(new QMenu(this));
    connect(menu(), SIGNAL(triggered(QAction*)), SLOT(itemSelected(QAction*)));
    setPopupMode(QToolButton::DelayedPopup);
    setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
}

QAction * Ui::NavButton::add(const QString &str, int id, const QIcon &icon) {
    QAction *act=menu()->addAction(str);
    act->setProperty(constIdProperty, id);
    act->setIcon(icon);
    setText(str);
    setIcon(icon);
    return act;
}

QAction * Ui::NavButton::add(const QString &str, const QModelIndex &idx, const QIcon &icon) {
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

    while (i.isValid()) {
        path=i.data().toString()+QLatin1String(" / ")+path;
        i=i.parent();
    }
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
    return FlatToolButton::sizeHint()+QSize(16, 8);
}

void Ui::NavButton::itemSelected(QAction *act) {
    if (act->property(constIdxProperty).isValid()) {
        emit selected(act->property(constIdxProperty).toModelIndex());
    } else {
        emit selected(act->property(constIdProperty).toInt());
    }
}
