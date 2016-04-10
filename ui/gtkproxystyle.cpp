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

#include "ui/gtkproxystyle.h"
#include "ui/gtkstyle.h"
#if defined HAVE_SHORTCUT_HANDLER
#include "ui/shortcuthandler.h"
#endif
#if defined HAVE_ACCEL_MGR
#include "ui/acceleratormanager.h"
#endif
#include "ui/utils.h"
#include <QSpinBox>
#include <QAbstractScrollArea>
#include <QAbstractItemView>
#include <QMenu>
#include <QToolBar>
#include <QComboBox>
#include <QScrollBar>
#include <QPainter>
#include <QStyleOptionSpinBox>
#include <QStyledItemDelegate>
#include <QApplication>
#include <QApplication>

#if defined HAVE_ACCEL_MGR
static const char * constAccelProp="catata-accel";
#endif

#if defined HAVE_SHORTCUT_HANDLER
static inline void addEventFilter(QObject *object, QObject *filter) {
    object->removeEventFilter(filter);
    object->installEventFilter(filter);
}
#endif

static bool useOverlayStyleScrollbars(bool use) {
    if (use) {
        QByteArray env=qgetenv("LIBOVERLAY_SCROLLBAR");
        if (!env.isEmpty() && env!="1") {
            return false;
        }
        QString mode=Ui::GtkStyle::readDconfSetting(QLatin1String("scrollbar-mode"), QLatin1String("/com/canonical/desktop/interface/"));
        return mode!=QLatin1String("normal");
    }
    return use;
}

static const char * constOnCombo="on-combo";

static bool isOnCombo(const QWidget *w)
{
    return w && (qobject_cast<const QComboBox *>(w) || isOnCombo(w->parentWidget()));
}

static void drawSpinButton(QPainter *painter, const QRect &r, const QColor &col, bool isPlus)
{
    int length=r.height()*0.5;
    int lineWidth=length<24 ? 2 : 4;
    if (length<(lineWidth*2)) {
        length=lineWidth*2;
    } else if (length%2) {
        length++;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing, false);
    painter->fillRect(r.x()+((r.width()-length)/2), r.y()+((r.height()-lineWidth)/2), length, lineWidth, col);
    if (isPlus) {
        painter->fillRect(r.x()+((r.width()-lineWidth)/2), r.y()+((r.height()-length)/2), lineWidth, length, col);
    }
    painter->restore();
}

Ui::GtkProxyStyle::GtkProxyStyle(int modView, bool thinSb, bool styleSpin, const QMap<QString, QString> &c)
    : ProxyStyle(modView)
    , css(c)
    , touchStyleSpin(styleSpin)
    , spinButtonRatio(1.25)
    , sbarPlainViewWidth(-1)
{
    #if defined HAVE_SHORTCUT_HANDLER
    shortcutHander=new ShortcutHandler(this);
    #endif
    setBaseStyle(qApp->style());

    if (useOverlayStyleScrollbars(thinSb)) {
        sbarType=SB_Gtk;
        sbarPlainViewWidth=QApplication::fontMetrics().height()/1.75;
    } else {
        sbarType=SB_Standard;
    }
    setModifyViewFrame(modView && (SB_Gtk==sbarType || !qApp->style()->styleHint(SH_ScrollView_FrameOnlyAroundContents, 0, 0, 0)) ? modView : 0);
}

Ui::GtkProxyStyle::~GtkProxyStyle() {
}

QSize Ui::GtkProxyStyle::sizeFromContents(ContentsType type, const QStyleOption *option,  const QSize &size, const QWidget *widget) const {
    QSize sz=baseStyle()->sizeFromContents(type, option, size, widget);

    if (SB_Standard!=sbarType && CT_ScrollBar==type) {
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            int extent(pixelMetric(PM_ScrollBarExtent, option, widget)),
                sliderMin(pixelMetric(PM_ScrollBarSliderMin, option, widget));

            if (sb->orientation == Qt::Horizontal) {
                sz = QSize(sliderMin, extent);
            } else {
                sz = QSize(extent, sliderMin);
            }
        }
    }

    if (touchStyleSpin && CT_SpinBox==type) {
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            if (QAbstractSpinBox::NoButtons!=spinBox->buttonSymbols) {
                #if QT_VERSION < 0x050200
                sz += QSize(0, 1);
                #endif
                // Qt5 does not seem to be taking special value, or suffix, into account when calculatng width...
                if (widget && qobject_cast<const QSpinBox *>(widget)) {
                    const QSpinBox *spin=static_cast<const QSpinBox *>(widget);
                    QString special=spin->specialValueText();
                    int minWidth=0;
                    if (!special.isEmpty()) {
                        minWidth=option->fontMetrics.width(special+QLatin1String(" "));
                    }

                    QString suffix=spin->suffix()+QLatin1String(" ");
                    minWidth=qMax(option->fontMetrics.width(QString::number(spin->minimum())+suffix), minWidth);
                    minWidth=qMax(option->fontMetrics.width(QString::number(spin->maximum())+suffix), minWidth);

                    if (minWidth>0) {
                        int frameWidth=baseStyle()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, 0);
                        int buttonWidth=(sz.height()-(frameWidth*2))*spinButtonRatio;
                        minWidth=((minWidth+(buttonWidth+frameWidth)*2)*1.05)+0.5;
                        if (sz.width()<minWidth) {
                            sz.setWidth(minWidth);
                        }
                    }
                } else if (!GtkStyle::isActive()) {
                    sz.setWidth(sz.width()+6);
                }
            }
        }
    }
    return sz;
}

int Ui::GtkProxyStyle::styleHint(StyleHint hint, const QStyleOption *option, const QWidget *widget, QStyleHintReturn *returnData) const {
    switch (hint) {
    case SH_DialogButtonBox_ButtonsHaveIcons:
        return false;
    #if defined HAVE_SHORTCUT_HANDLER
    case SH_UnderlineShortcut:
        return widget ? shortcutHander->showShortcut(widget) : true;
    #endif
    case SH_ScrollView_FrameOnlyAroundContents:
        if (SB_Standard!=sbarType) {
            return false;
        }
        break;
    default:
        break;
    }

    return ProxyStyle::styleHint(hint, option, widget, returnData);
}

int Ui::GtkProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption *option, const QWidget *widget) const {
    if (SB_Standard!=sbarType && PM_ScrollBarExtent==metric) {
        return sbarPlainViewWidth;
    }
    return baseStyle()->pixelMetric(metric, option, widget);
}

QRect Ui::GtkProxyStyle::subControlRect(ComplexControl control, const QStyleOptionComplex *option, SubControl subControl, const QWidget *widget) const {
    if (SB_Standard!=sbarType && CC_ScrollBar==control) {
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(option))  {
            QRect ret;
            bool  horizontal(Qt::Horizontal==sb->orientation);
            int   sbextent(pixelMetric(PM_ScrollBarExtent, sb, widget)),
                  sliderMaxLength(horizontal ? sb->rect.width() : sb->rect.height()),
                  sliderMinLength(pixelMetric(PM_ScrollBarSliderMin, sb, widget)),
                  sliderLength;

            if (sb->maximum != sb->minimum) {
                uint valueRange = sb->maximum - sb->minimum;
                sliderLength = (sb->pageStep * sliderMaxLength) / (valueRange + sb->pageStep);

                if (sliderLength < sliderMinLength) {
                    sliderLength = sliderMinLength;
                }
                if (sliderLength > sliderMaxLength) {
                    sliderLength = sliderMaxLength;
                }
            } else {
                sliderLength = sliderMaxLength;
            }

            int sliderstart(sliderPositionFromValue(sb->minimum, sb->maximum, sb->sliderPosition, sliderMaxLength - sliderLength, sb->upsideDown));

            // Subcontrols
            switch(subControl)
            {
            case SC_ScrollBarSubLine:
            case SC_ScrollBarAddLine:
                return QRect();
            case SC_ScrollBarSubPage:
                if (horizontal) {
                    ret.setRect(0, 0, sliderstart, sbextent);
                } else {
                    ret.setRect(0, 0, sbextent, sliderstart);
                }
                break;
            case SC_ScrollBarAddPage:
                if (horizontal) {
                    ret.setRect(sliderstart + sliderLength, 0, sliderMaxLength - sliderstart - sliderLength, sbextent);
                } else {
                    ret.setRect(0, sliderstart + sliderLength, sbextent, sliderMaxLength - sliderstart - sliderLength);
                }
                break;
            case SC_ScrollBarGroove:
                ret=QRect(0, 0, sb->rect.width(), sb->rect.height());
                break;
            case SC_ScrollBarSlider:
                if (horizontal) {
                    ret=QRect(sliderstart, 0, sliderLength, sbextent);
                } else {
                    ret=QRect(0, sliderstart, sbextent, sliderLength);
                }
                break;
            default:
                ret = baseStyle()->subControlRect(control, option, subControl, widget);
                break;
            }
            return visualRect(sb->direction/*Qt::LeftToRight*/, sb->rect, ret);
        }
    }

    if (touchStyleSpin && CC_SpinBox==control) {
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            if (QAbstractSpinBox::NoButtons!=spinBox->buttonSymbols) {
                int border=2;
                int padBeforeButtons=GtkStyle::isActive() ? 0 : 2;
                int internalHeight=spinBox->rect.height()-(border*2);
                int internalWidth=internalHeight*spinButtonRatio;
                switch (subControl) {
                case SC_SpinBoxUp:
                    return Qt::LeftToRight==spinBox->direction
                                ? QRect(spinBox->rect.width()-(internalWidth+border), border, internalWidth, internalHeight)
                                : QRect(border, border, internalWidth, internalHeight);
                case SC_SpinBoxDown:
                    return Qt::LeftToRight==spinBox->direction
                                ? QRect(spinBox->rect.width()-((internalWidth*2)+border), border, internalWidth, internalHeight)
                                : QRect(internalWidth+border, border, internalWidth, internalHeight);
                case SC_SpinBoxEditField:
                    return Qt::LeftToRight==spinBox->direction
                            ? QRect(border, border, spinBox->rect.width()-((internalWidth*2)+border+padBeforeButtons), internalHeight)
                            : QRect(((internalWidth*2)+border), border, spinBox->rect.width()-((internalWidth*2)+border+padBeforeButtons), internalHeight);
                case SC_SpinBoxFrame:
                    return spinBox->rect;
                default:
                    break;
                }
            }
        }
    }
    return baseStyle()->subControlRect(control, option, subControl, widget);
}

void Ui::GtkProxyStyle::drawComplexControl(ComplexControl control, const QStyleOptionComplex *option, QPainter *painter, const QWidget *widget) const {
    if (SB_Standard!=sbarType && CC_ScrollBar==control) {
        if (const QStyleOptionSlider *sb = qstyleoption_cast<const QStyleOptionSlider *>(option)) {
            QRect r=option->rect;
            QRect slider=subControlRect(control, option, SC_ScrollBarSlider, widget);
            if (widget && widget->property(constOnCombo).toBool()) {
                painter->fillRect(r, QApplication::palette().color(QPalette::Background)); // option->palette.background());
            } else if (!widget || widget->testAttribute(Qt::WA_OpaquePaintEvent)) {
                if (option->palette.base().color()==Qt::transparent) {
                    painter->fillRect(r, QApplication::palette().color(QPalette::Base));
                } else {
                    painter->fillRect(r, option->palette.base());
                }
            }

            if (slider.isValid()) {
                bool inactive=!(sb->activeSubControls&SC_ScrollBarSlider && (option->state&State_MouseOver || option->state&State_Sunken));
                #ifdef Q_OS_MAC
                QColor col(OSXStyle::self()->viewPalette().highlight().color());
                #else
                QColor col(option->palette.highlight().color());
                #endif
                if (!(option->state&State_Active)) {
                    col=col.darker(115);
                }
                if (SB_Gtk==sbarType) {
                    int adjust=inactive ? 3 : 1;
                    if (Qt::Horizontal==sb->orientation) {
                        slider.adjust(1, adjust, -1, -adjust);
                    } else {
                        slider.adjust(adjust, 1, -adjust, -1);
                    }
                    int dimension=(Qt::Horizontal==sb->orientation ? slider.height() : slider.width());
                    QPainterPath path=Ui::Utils::buildPath(QRectF(slider.x()+0.5, slider.y()+0.5, slider.width()-1, slider.height()-1),
                                                           dimension>6 ? (dimension/4.0) : (dimension/8.0));
                    painter->save();
                    painter->setRenderHint(QPainter::Antialiasing, true);
                    painter->fillPath(path, col);
                    painter->setPen(col);
                    painter->drawPath(path);
                    painter->restore();
                } else {
                    painter->fillRect(slider, col);
                }
            }
            return;
        }
    }

    if (touchStyleSpin && CC_SpinBox==control) {
        if (const QStyleOptionSpinBox *spinBox = qstyleoption_cast<const QStyleOptionSpinBox *>(option)) {
            if (QAbstractSpinBox::NoButtons!=spinBox->buttonSymbols) {
                QStyleOptionFrame opt;
                opt.state=spinBox->state;
                opt.state|=State_Sunken;
                opt.rect=spinBox->rect;
                opt.palette=spinBox->palette;
                opt.lineWidth=baseStyle()->pixelMetric(QStyle::PM_DefaultFrameWidth, option, widget);
                opt.midLineWidth=0;
                opt.fontMetrics=spinBox->fontMetrics;
                opt.direction=spinBox->direction;
                baseStyle()->drawPrimitive(PE_PanelLineEdit, &opt, painter, 0);

                QRect plusRect=subControlRect(CC_SpinBox, spinBox, SC_SpinBoxUp, widget);
                QRect minusRect=subControlRect(CC_SpinBox, spinBox, SC_SpinBoxDown, widget);
                QColor separatorColor(spinBox->palette.foreground().color());
                separatorColor.setAlphaF(0.15);
                painter->setPen(separatorColor);
                if (Qt::LeftToRight==spinBox->direction) {
                    painter->drawLine(plusRect.topLeft(), plusRect.bottomLeft());
                    painter->drawLine(minusRect.topLeft(), minusRect.bottomLeft());
                } else {
                    painter->drawLine(plusRect.topRight(), plusRect.bottomRight());
                    painter->drawLine(minusRect.topRight(), minusRect.bottomRight());
                }

                if (option->state&State_Sunken) {
                    QRect fillRect;

                    if (spinBox->activeSubControls&SC_SpinBoxUp) {
                        fillRect=plusRect;
                    } else if (spinBox->activeSubControls&SC_SpinBoxDown) {
                        fillRect=minusRect;
                    }
                    if (!fillRect.isEmpty()) {
                        QColor col=spinBox->palette.highlight().color();
                        col.setAlphaF(0.1);
                        painter->fillRect(fillRect.adjusted(1, 1, -1, -1), col);
                    }
                }

                drawSpinButton(painter, plusRect,
                               spinBox->palette.color(spinBox->state&State_Enabled && (spinBox->stepEnabled&QAbstractSpinBox::StepUpEnabled)
                                ? QPalette::Current : QPalette::Disabled, QPalette::Text), true);
                drawSpinButton(painter, minusRect,
                               spinBox->palette.color(spinBox->state&State_Enabled && (spinBox->stepEnabled&QAbstractSpinBox::StepDownEnabled)
                                ? QPalette::Current : QPalette::Disabled, QPalette::Text), false);
                return;
            }
        }
    }
    baseStyle()->drawComplexControl(control, option, painter, widget);
}

void Ui::GtkProxyStyle::drawPrimitive(PrimitiveElement element, const QStyleOption *option, QPainter *painter, const QWidget *widget) const {
    if (PE_PanelScrollAreaCorner==element && option && SB_Standard!=sbarType) {
        painter->fillRect(option->rect, option->palette.brush(QPalette::Base));
    } else {
        ProxyStyle::drawPrimitive(element, option, painter, widget);
    }
}

void Ui::GtkProxyStyle::polish(QWidget *widget) {
    #if defined HAVE_ACCEL_MGR
    if (widget && qobject_cast<QMenu *>(widget) && !widget->property(constAccelProp).isValid()) {
        AcceleratorManager::manage(widget);
        widget->setProperty(constAccelProp, true);
    }
    #endif

    // Apply CSS only to particular widgets. With Qt5.2 if we apply CSS to whole application, then QStyleSheetStyle does
    // NOT call sizeFromContents for spinboxes :-(
    if (widget->styleSheet().isEmpty()) {
        QMap<QString, QString>::ConstIterator it=css.end();
        if (qobject_cast<QToolBar *>(widget)) {
            it=css.find(QLatin1String("QToolBar#")+widget->objectName());
        } else if (qobject_cast<QMenu *>(widget)) {
            it=css.find(QLatin1String(widget->metaObject()->className()));
        }
        if (css.end()!=it) {
            widget->setStyleSheet(it.value());
        }
    }

    if (SB_Standard!=sbarType) {
        if (qobject_cast<QScrollBar *>(widget)) {
            if (isOnCombo(widget)) {
                widget->setProperty(constOnCombo, true);
            }
        } else if (qobject_cast<QAbstractScrollArea *>(widget) && widget->inherits("QComboBoxListView")) {
            QAbstractScrollArea *sa=static_cast<QAbstractScrollArea *>(widget);
            QWidget *sb=sa->horizontalScrollBar();
            if (sb) {
                sb->setProperty(constOnCombo, true);
            }
            sb=sa->verticalScrollBar();
            if (sb) {
                sb->setProperty(constOnCombo, true);
            }
        }
    }

    ProxyStyle::polish(widget);
}

void Ui::GtkProxyStyle::polish(QPalette &pal) {
    ProxyStyle::polish(pal);
}

void Ui::GtkProxyStyle::polish(QApplication *app) {
    #if defined HAVE_SHORTCUT_HANDLER
    addEventFilter(app, shortcutHander);
    #endif
    ProxyStyle::polish(app);
}

void Ui::GtkProxyStyle::unpolish(QWidget *widget) {
    ProxyStyle::unpolish(widget);
}

void Ui::GtkProxyStyle::unpolish(QApplication *app) {
    #if defined HAVE_SHORTCUT_HANDLER
    app->removeEventFilter(shortcutHander);
    #endif
    ProxyStyle::unpolish(app);
}
