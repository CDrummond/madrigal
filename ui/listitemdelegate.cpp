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

#include "ui/listitemdelegate.h"
#include "ui/listview.h"
#include "ui/gtkstyle.h"
#include "ui/utils.h"
#include "core/actions.h"
#include "core/roles.h"
#include "core/images.h"
#include "core/utils.h"
#include "upnp/renderer.h"
#include <QPainter>
#include <QApplication>
#include <QHelpEvent>
#include <QToolTip>

static int listIconSize=1;
static int listCoverSize=-1;
static int gridCoverSize=-1;
static int borderSize=1;
static int actionBorder = 4;
static int actionIconSize = 16;

static int standardIconSize(int v) {
    if (v>48) {
        static const int constStep=4;
        return (((int)(v/constStep))*constStep)+((v%constStep) ? constStep : 0);
    } else if (v<19) {
        return 16;
    } else if (v<=28) {
        return 22;
    } else if (v<=40) {
        return 32;
    }
    return v;
}

static void setupSizes() {
    if (-1==listCoverSize) {
        int height=QApplication::fontMetrics().height();
        listIconSize=qMax(22, standardIconSize(2*height));
        listCoverSize=qMax(32, standardIconSize(2.5*height));
        gridCoverSize=qMax(128, standardIconSize(8.5*height));
        borderSize=listCoverSize>48 ? 2 : 1;

        if (height>22) {
            actionIconSize=standardIconSize(((int)(height/4))*4);
            actionBorder=actionIconSize>32 ? 8 : 6;
        } else {
            actionBorder=6;
            actionIconSize=22;
            borderSize=1;
        }
    }
}

static inline double subTextAlpha(bool selected) {
    return selected ? 0.7 : 0.5;
}

void Ui::ListItemDelegate::getSizes(int &border, int &listCover) {
    setupSizes();
    border=borderSize;
    listCover=listCoverSize;
}

void Ui::ListItemDelegate::drawPlayState(QPainter *painter, const QStyleOptionViewItem &option, const QRect &r, int state) {
    if (state) {
        int size=listIconSize > 32 ? 26 : 13;
        int hSize=size/2;
        QRect ir(r.x()-(size+6), r.y()+(((r.height()-size)/2.0)+0.5), size, size);
        QColor inside(option.palette.color(QPalette::Text));
        QColor border=inside.red()>100 && inside.blue()>100 && inside.green()>100 ? Qt::black : Qt::white;
        if (QApplication::isRightToLeft()) {
            ir.adjust(r.width()-size, 0, r.width()-size, 0);
        }

        switch (state) {
        case Upnp::Renderer::Stopped:
            painter->fillRect(ir, border);
            painter->fillRect(ir.adjusted(1, 1, -1, -1), inside);
            break;
        case Upnp::Renderer::Paused: {
            int blockSize=hSize-1;
            painter->fillRect(ir, border);
            painter->fillRect(ir.x()+1, ir.y()+1, blockSize, size-2, inside);
            painter->fillRect(ir.x()+size-blockSize-1, ir.y()+1, blockSize, size-2, inside);
            break;
        }
        case Upnp::Renderer::Playing: {
            ir.adjust(2, 0, -2, 0);
            QPoint p1[5]={ QPoint(ir.x()-2, ir.y()-1), QPoint(ir.x(), ir.y()-1), QPoint(ir.x()+(size-hSize), ir.y()+hSize), QPoint(ir.x(), ir.y()+(ir.height()-1)), QPoint(ir.x()-2, ir.y()+(ir.height()-1)) };
            QPoint p2[5]={ QPoint(ir.x()-2, ir.y()-1), QPoint(ir.x(), ir.y()-1), QPoint(ir.x()+(size-hSize), ir.y()+hSize), QPoint(ir.x(), ir.y()+ir.height()), QPoint(ir.x()-2, ir.y()+ir.height()) };
            painter->save();
            painter->setBrush(border);
            painter->setPen(border);
            painter->drawPolygon(p1, 5);
            painter->setBrush(inside);
            painter->drawPolygon(p2, 5);
            painter->restore();
            break;
        }
        }
    }
}

Ui::ListItemDelegate::ListItemDelegate(ListView *v)
    : BasicItemDelegate(v)
    , view(v)
{
}

Ui::ListItemDelegate::~ListItemDelegate() {
}

QSize Ui::ListItemDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)
    ::setupSizes();
    if (view && QListView::IconMode==view->viewMode()) {
        return QSize(gridCoverSize+8, gridCoverSize+(QApplication::fontMetrics().height()*2.5));
    } else {
        int textHeight = QApplication::fontMetrics().height()*2;

        return QSize(qMax(64, listCoverSize) + (borderSize * 2),
                     qMax(textHeight, listCoverSize) + (borderSize*2));
    }
}

void Ui::ListItemDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const {
    if (!index.isValid()) {
        return;
    }
    ::setupSizes();
    bool mouseOver=option.state&QStyle::State_MouseOver;
    bool gtk=mouseOver && GtkStyle::isActive();
    bool selected=option.state&QStyle::State_Selected;
    bool active=option.state&QStyle::State_Active;
    bool drawBgnd=true;
    bool iconMode = view && QListView::IconMode==view->viewMode();
    QStyleOptionViewItemV4 opt(option);
    opt.showDecorationSelected=true;

    if (!underMouse) {
        if (mouseOver && !selected) {
            drawBgnd=false;
        }
        mouseOver=false;
    }

    if (drawBgnd) {
        if (mouseOver && gtk) {
            GtkStyle::drawSelection(opt, painter, selected ? 0.75 : 0.25);
        } else {
            QApplication::style()->drawPrimitive(QStyle::PE_PanelItemViewItem, &opt, painter, view);
        }
    }

    QString text = iconMode ? index.data(Core::Role_MainText).toString() : QString();
    if (text.isEmpty()) {
        text=index.data(Qt::DisplayRole).toString();
    }
    QRect r(option.rect);
    QRect r2(r);
    QString childText = index.data(Core::Role_SubText).toString();
    QPixmap pix;
    QString otherText = index.data(Core::Role_OtherText).toString();
    bool isCurrent = index.data(Core::Role_IsCurrent).toBool();
    Core::ImageDetails cover=index.data(Core::Role_ImageDetails).value<Core::ImageDetails>();
    if (!cover.url.isEmpty()) {
        QImage *img=Core::Images::self()->get(cover, iconMode ? gridCoverSize : listCoverSize);
        if (img) {
            pix=QPixmap::fromImage(*img);
        }
    }
    if (pix.isNull()) {
        int size=iconMode ? gridCoverSize : listIconSize;
        pix=index.data(Qt::DecorationRole).value<QIcon>().pixmap(size, size);
    }

    bool oneLine = childText.isEmpty();
    bool rtl = QApplication::isRightToLeft();

    if (childText==QLatin1String("-")) {
        childText.clear();
    }

    painter->save();
    painter->setClipRect(r);

    QFont textFont(index.data(Qt::FontRole).value<QFont>());
    textFont.setBold(isCurrent);
    QFontMetrics textMetrics(textFont);
    int textHeight=textMetrics.height();

    if (iconMode) {
        r.adjust(borderSize, borderSize*4, -borderSize, -borderSize);
        r2=r;
    } else {
        r.adjust(borderSize, 0, -borderSize, 0);
    }
    if (!pix.isNull()) {
        #if QT_VERSION >= 0x050100
        QSize layoutSize = pix.size() / pix.devicePixelRatio();
        #else
        QSize layoutSize = pix.size();
        #endif
        int adjust=qMax(layoutSize.width(), layoutSize.height());
        if (iconMode) {
            int xpos=r.x()+((r.width()-layoutSize.width())/2);
            painter->drawPixmap(xpos, r.y(), layoutSize.width(), layoutSize.height(), pix);
            QColor color(option.palette.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Text));
            double alphas[]={0.25, 0.125, 0.061};
            QRect border(xpos, r.y(), layoutSize.width(), layoutSize.height());
            QRect shadow(border);
            for (int i=0; i<3; ++i) {
                shadow.adjust(1, 1, 1, 1);
                color.setAlphaF(alphas[i]);
                painter->setPen(color);
                painter->drawLine(shadow.bottomLeft()+QPoint(i+1, 0),
                                  shadow.bottomRight()+QPoint(-((i*2)+2), 0));
                painter->drawLine(shadow.bottomRight()+QPoint(0, -((i*2)+2)),
                                  shadow.topRight()+QPoint(0, i+1));
                if (1==i) {
                    painter->drawPoint(shadow.bottomRight()-QPoint(2, 1));
                    painter->drawPoint(shadow.bottomRight()-QPoint(1, 2));
                    painter->drawPoint(shadow.bottomLeft()-QPoint(1, 1));
                    painter->drawPoint(shadow.topRight()-QPoint(1, 1));
                } else if (2==i) {
                    painter->drawPoint(shadow.bottomRight()-QPoint(4, 1));
                    painter->drawPoint(shadow.bottomRight()-QPoint(1, 4));
                    painter->drawPoint(shadow.bottomLeft()-QPoint(0, 1));
                    painter->drawPoint(shadow.topRight()-QPoint(1, 0));
                    painter->drawPoint(shadow.bottomRight()-QPoint(2, 2));
                }
            }
            color.setAlphaF(0.4);
            painter->setPen(color);
            painter->drawRect(border.adjusted(0, 0, -1, -1));
            r.adjust(0, adjust+3, 0, -3);
        } else {
            if (rtl) {
                painter->drawPixmap(r.x()+r.width()-layoutSize.width(), r.y()+((r.height()-layoutSize.height())/2), layoutSize.width(), layoutSize.height(), pix);
                r.adjust(3, 0, -(3+adjust), 0);
            } else {
                painter->drawPixmap(r.x(), r.y()+((r.height()-layoutSize.height())/2), layoutSize.width(), layoutSize.height(), pix);
                r.adjust(adjust+3, 0, -3, 0);
            }
        }
    }

    int otherTextWidth = otherText.isEmpty() ? 0 : textMetrics.width(otherText+" ");
    #ifdef Q_OS_WIN
    QColor color(option.palette.color(active ? QPalette::Active : QPalette::Inactive, QPalette::Text));
    #else
    QColor color(option.palette.color(active ? QPalette::Active : QPalette::Inactive, selected ? QPalette::HighlightedText : QPalette::Text));
    #endif
    QTextOption textOpt(iconMode ? Qt::AlignHCenter|Qt::AlignVCenter : Qt::AlignVCenter);

    textOpt.setWrapMode(QTextOption::NoWrap);
    QRect textRect;
    if (oneLine) {
        textRect=QRect(r.x(), r.y()+((r.height()-textHeight)/2), r.width()-otherTextWidth, textHeight);
        text = textMetrics.elidedText(text, Qt::ElideRight, textRect.width(), QPalette::WindowText);
        painter->setPen(color);
        painter->setFont(textFont);
        painter->drawText(textRect, text, textOpt);
        if (0!=otherTextWidth) {
            textRect=QRect(r.x()+r.width()-otherTextWidth, textRect.y(), otherTextWidth, textHeight);
            painter->drawText(textRect, otherText, textOpt);
        }
    } else {
        QFont childFont(Utils::smallFont(textFont));
        childFont.setBold(false);
        QFontMetrics childMetrics(childFont);

        int childHeight=childMetrics.height();
        int totalHeight=textHeight+childHeight;
        textRect=QRect(r.x(), r.y()+((r.height()-totalHeight)/2), r.width()-otherTextWidth, textHeight);
        QRect childRect(r.x(), r.y()+textHeight+((r.height()-totalHeight)/2), r.width(),
                        (iconMode ? childHeight-(2*borderSize) : childHeight));

        text = textMetrics.elidedText(text, Qt::ElideRight, textRect.width(), QPalette::WindowText);
        painter->setPen(color);
        painter->setFont(textFont);
        painter->drawText(textRect, text, textOpt);
        if (0!=otherTextWidth) {
            textRect=QRect(r.x()+r.width()-otherTextWidth, textRect.y(), otherTextWidth, textHeight);
            painter->drawText(textRect, otherText, textOpt);
        }

        if (!childText.isEmpty()) {
            childText = childMetrics.elidedText(childText, Qt::ElideRight, childRect.width(), QPalette::WindowText);
            color.setAlphaF(subTextAlpha(selected));
            painter->setPen(color);
            painter->setFont(childFont);
            painter->drawText(childRect, childText, textOpt);
        }
    }

    if (drawBgnd && mouseOver) {
        QList<int> actions=index.data(Core::Role_Actions).value< QList<int> >();
        if (!actions.isEmpty()) {
            drawIcons(painter, iconMode ? option.rect : r, mouseOver, rtl, iconMode, index);
        }
    }
    if (!iconMode) {
        BasicItemDelegate::drawLine(painter, option.rect, color);
        if (isCurrent) {
            drawPlayState(painter, option, textRect, index.data(Core::Role_PlayState).toInt());
        }
    }
    painter->restore();
}

QRect Ui::ListItemDelegate::calcActionRect(bool rtl, bool onTop, const QRect &rect) const
{
    return rtl
                ? onTop
                    ? QRect(rect.x()+(borderSize*4)+4,
                            rect.y()+(borderSize*4)+4,
                            actionIconSize, actionIconSize)
                    : QRect(rect.x()+actionBorder,
                            rect.y()+((rect.height()-actionIconSize)/2),
                            actionIconSize, actionIconSize)
                : onTop
                    ? QRect(rect.x()+rect.width()-(actionIconSize+(borderSize*4))-4,
                            rect.y()+(borderSize*4)+4,
                            actionIconSize, actionIconSize)
                    : QRect(rect.x()+rect.width()-(actionIconSize+actionBorder),
                            rect.y()+((rect.height()-actionIconSize)/2),
                            actionIconSize, actionIconSize);
}

void Ui::ListItemDelegate::adjustActionRect(bool rtl, bool onTop, QRect &rect)
{
    if (rtl) {
        if (onTop) {
            rect.adjust(0, actionIconSize+actionBorder, 0, actionIconSize+actionBorder);
        } else {
            rect.adjust(actionIconSize+actionBorder, 0, actionIconSize+actionBorder, 0);
        }
    } else {
        if (onTop) {
            rect.adjust(0, actionIconSize+actionBorder, 0, actionIconSize+actionBorder);
        } else {
            rect.adjust(-(actionIconSize+actionBorder), 0, -(actionIconSize+actionBorder), 0);
        }
    }
}

static void drawBgnd(QPainter *painter, const QRect &rx, bool light)
{
    QRectF r(rx.x()-0.5, rx.y()-0.5, rx.width()+1, rx.height()+1);
    QPainterPath p(Ui::Utils::buildPath(r, 3.0));
    QColor c(light ? Qt::white : Qt::black);

    painter->setRenderHint(QPainter::Antialiasing, true);
    c.setAlphaF(0.75);
    painter->fillPath(p, c);
    c.setAlphaF(0.95);
    painter->setPen(c);
    painter->drawPath(p);
    painter->setRenderHint(QPainter::Antialiasing, false);
}

void Ui::ListItemDelegate::drawIcons(QPainter *painter, const QRect &r, bool mouseOver, bool rtl, bool onTop, const QModelIndex &index) const
{
    QColor textCol=QApplication::palette().color(QPalette::Normal, QPalette::WindowText);
    bool lightBgnd=textCol.red()<=100 && textCol.green()<=100 && textCol.blue()<=100;
    double opacity=painter->opacity();
    bool adjustOpacity=!mouseOver;
    if (adjustOpacity) {
        painter->setOpacity(opacity*0.25);
    }
    QRect actionRect=calcActionRect(rtl, onTop, r);
    QList<int> actions=index.data(Core::Role_Actions).value< QList<int> >();

    foreach (int a, actions) {
        QPixmap pix=Core::Actions::icon((Core::Actions::Type)a).pixmap(QSize(actionIconSize, actionIconSize));
        QSize pixSize = pix.isNull() ? QSize(0, 0) : (pix.size() / pix.devicePixelRatio());

        if (!pix.isNull() && actionRect.width()>=pixSize.width()/* && r.x()>=0 && r.y()>=0*/) {
            drawBgnd(painter, actionRect, lightBgnd);
            painter->drawPixmap(actionRect.x()+(actionRect.width()-pixSize.width())/2,
                                actionRect.y()+(actionRect.height()-pixSize.height())/2, pix);
        }
        adjustActionRect(rtl, onTop, actionRect);
    }

    if (adjustOpacity) {
        painter->setOpacity(opacity);
    }
}

bool Ui::ListItemDelegate::helpEvent(QHelpEvent *e, QAbstractItemView *view, const QStyleOptionViewItem &option, const QModelIndex &index)
{
    if (QEvent::ToolTip==e->type()) {
        int act=getAction(index);
        if (Core::Actions::Action_None!=act) {
            QToolTip::showText(e->globalPos(), Core::Actions::toolTip((Core::Actions::Type)act), view);
            return true;
        }
    }
    return QStyledItemDelegate::helpEvent(e, view, option, index);
}

int Ui::ListItemDelegate::getAction(const QModelIndex &index) const
{
    QList<int> actions=index.data(Core::Role_Actions).value< QList<int> >();
    if (actions.isEmpty()) {
        return 0;
    }

    bool rtl = QApplication::isRightToLeft();
    bool onTop=QListView::IconMode==view->viewMode();
    QRect rect = view->visualRect(index);
    rect.moveTo(view->viewport()->mapToGlobal(QPoint(rect.x(), rect.y())));
    if (onTop) {
        rect.adjust(borderSize, borderSize, -borderSize, -borderSize);
    } else {
        rect.adjust(borderSize+3, 0, -(borderSize+3), 0);
    }

    QRect actionRect=calcActionRect(rtl, onTop, rect);
    QRect actionRect2(actionRect);
    adjustActionRect(rtl, onTop, actionRect2);
    QPoint cursorPos=QCursor::pos();

    foreach (int a, actions) {
        actionRect=onTop ? actionRect.adjusted(0, -2, 0, 2) : actionRect.adjusted(-2, 0, 2, 0);
        if (actionRect.contains(cursorPos)) {
            return a;
        }

        adjustActionRect(rtl, onTop, actionRect);
    }

    return Core::Actions::Action_None;
}
