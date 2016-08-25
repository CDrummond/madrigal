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

#include "ui/volumeslider.h"
#include "ui/utils.h"
#include "ui/action.h"
#include "ui/actioncollection.h"
#include <QStyle>
#include <QPainter>
#include <QPainterPath>
#include <QProxyStyle>
#include <QApplication>
#include <QLabel>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QMenu>

class VolumeSliderProxyStyle : public QProxyStyle {
public:
    VolumeSliderProxyStyle()
        : QProxyStyle()
    {
        setBaseStyle(qApp->style());
    }

    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const {
        if (SH_Slider_AbsoluteSetButtons==stylehint) {
            return Qt::LeftButton|QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        } else {
            return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        }
    }
};

static int widthStep=4;
static int constHeightStep=2;

Ui::VolumeSlider::VolumeSlider(QWidget *p)
    : QSlider(p)
    , lineWidth(0)
    , down(false)
    , isMuted(false)
    , muteAction(0)
    , menu(0)
{
    widthStep=4;
    setRange(0, 100);
    setPageStep(5);
    lineWidth=Utils::scaleForDpi(1);

    int w=lineWidth*widthStep*19;
    int h=lineWidth*constHeightStep*10;
    setFixedHeight(h+1);
    setFixedWidth(w);
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setOrientation(Qt::Horizontal);
    setFocusPolicy(Qt::NoFocus);
    setStyle(new VolumeSliderProxyStyle());
    setStyleSheet(QString("QSlider::groove:horizontal {border: 0px;} "
                          "QSlider::sub-page:horizontal {border: 0px;} "
                          "QSlider::handle:horizontal {width: 0px; height:0px; margin:0;}"));
    textCol=Utils::clampColor(palette().color(QPalette::Active, QPalette::Text));
    generatePixmaps();

    muteAction = ActionCollection::get()->createAction("mute", tr("Mute"));
    increaseAction = ActionCollection::get()->createAction("incvol", tr("Increase Volume"));
    decreaseAction = ActionCollection::get()->createAction("decvol", tr("Decrease Volume"));
    muteAction->setShortcut(Qt::ControlModifier+Qt::Key_M);
    increaseAction->setShortcut(Qt::ControlModifier+Qt::Key_Up);
    decreaseAction->setShortcut(Qt::ControlModifier+Qt::Key_Down);

    addAction(muteAction);
    connect(muteAction, SIGNAL(triggered()), this, SLOT(muteTriggered()));
    connect(increaseAction, SIGNAL(triggered()), this, SLOT(increaseVolume()));
    connect(decreaseAction, SIGNAL(triggered()), this, SLOT(decreaseVolume()));
    connect(this, SIGNAL(valueChanged(int)), this, SLOT(changeVolume()));
    addAction(increaseAction);
    addAction(decreaseAction);
}

void Ui::VolumeSlider::setColor(QColor col) {
    col=Utils::clampColor(col);
    if (col!=textCol) {
        textCol=col;
        generatePixmaps();
    }
}

void Ui::VolumeSlider::paintEvent(QPaintEvent *) {
    bool reverse=isRightToLeft();
    QPainter p(this);
    if (isMuted || !isEnabled()) {
        p.setOpacity(0.25);
    }

    p.drawPixmap(0, 0, pixmaps[0]);
    int percent=((value()*100.0)/maximum())+0.5;

    #if 1
    int steps=(percent/10.0)+0.5;
    if (steps>0) {
        if (steps<10) {
            int wStep=widthStep*lineWidth;
            p.setClipRect(reverse
                            ? QRect(width()-((steps*wStep*2)-wStep), 0, width(), height())
                            : QRect(0, 0, (steps*wStep*2)-wStep, height()));
            p.setClipping(true);
        }
        p.drawPixmap(0, 0, pixmaps[1]);
        if (steps<10) {
            p.setClipping(false);
        }
    }
    #else // Partial filling of each block?
    if (value()>0) {
        if (value()<100) {
            int fillWidth=(width()*(0.01*percent))+0.5;
            p.setClipRect(reverse
                            ? QRect(width()-fillWidth, 0, width(), height())
                            : QRect(0, 0, fillWidth, height()));
            p.setClipping(true);
        }
        p.drawPixmap(0, 0, *(pixmaps[1]));
        if (value()<100) {
            p.setClipping(false);
        }
    }
    #endif

    if (!isMuted) {
        p.setOpacity(p.opacity()*0.75);
        p.setPen(textCol);
        QFont f(font());
        f.setPixelSize(qMax(height()/2.5, 8.0));
        p.setFont(f);
        QRect r=rect();
        bool rtl=isRightToLeft();
        if (rtl) {
            r.setX(widthStep*lineWidth*12);
        } else {
            r.setWidth(widthStep*lineWidth*7);
        }
        p.drawText(r, Qt::AlignRight, QString("%1%").arg(percent));
    }
}

void Ui::VolumeSlider::mousePressEvent(QMouseEvent *ev) {
    if (Qt::MiddleButton==ev->buttons()) {
        down=true;
    } else {
        QSlider::mousePressEvent(ev);
    }
}

void Ui::VolumeSlider::mouseReleaseEvent(QMouseEvent *ev) {
    if (down) {
        down=false;
        muteAction->trigger();
        update();
    } else {
        QSlider::mouseReleaseEvent(ev);
    }
}

void Ui::VolumeSlider::contextMenuEvent(QContextMenuEvent *ev) {
    static const char *constValProp="val";
    if (!menu) {
        menu=new QMenu(this);
        muteMenuAction=menu->addAction(tr("Mute"));
        muteMenuAction->setProperty(constValProp, -1);
        for (int i=0; i<11; ++i) {
            menu->addAction(QString("%1%").arg(i*10))->setProperty(constValProp, i*10);
        }
    }

    muteMenuAction->setText(isMuted ? tr("Unmute") : tr("Mute"));
    QAction *ret = menu->exec(mapToGlobal(ev->pos()));
    if (ret) {
        int val=ret->property(constValProp).toInt();
        if (-1==val) {
            muteAction->trigger();
        } else {
            setValue(((val/100.0)*maximum())+0.5);
            emit setVolume(value());
        }
    }
}

void Ui::VolumeSlider::wheelEvent(QWheelEvent *ev) {
    int numDegrees = ev->delta() / 8;
    int numSteps = numDegrees / 15;
    if (numSteps > 0) {
        for (int i = 0; i < numSteps; ++i) {
            increaseVolume();
        }
    } else {
        for (int i = 0; i > numSteps; --i) {
            decreaseVolume();
        }
    }
}

void Ui::VolumeSlider::set(const Upnp::Renderer::Volume &vol) {
    isMuted=vol.muted;
    setMaximum(vol.max>0 ? vol.max : 100);
    blockSignals(true);
    if (vol.max<=0) {
        setValue(0);
        setEnabled(false);
    } else {
        setEnabled(true);
        int pc=((vol.current*100.0)/vol.max)+0.5;
        setToolTip(isMuted>0 ? tr("Volume %1% (Muted)").arg(pc) : tr("Volume %1%").arg(pc));
        setValue(vol.current);
    }
    update();
    blockSignals(false);
}

void Ui::VolumeSlider::increaseVolume() {
    triggerAction(QAbstractSlider::SliderPageStepAdd);
}

void Ui::VolumeSlider::decreaseVolume() {
    triggerAction(QAbstractSlider::SliderPageStepSub);
}

void Ui::VolumeSlider::changeVolume() {
    emit setVolume(value());
}

void Ui::VolumeSlider::muteTriggered() {
    emit mute(!isMuted);
}

void Ui::VolumeSlider::generatePixmaps() {
    pixmaps[0]=generatePixmap(false);
    pixmaps[1]=generatePixmap(true);
}

QPixmap Ui::VolumeSlider::generatePixmap(bool filled) {
    bool reverse=isRightToLeft();
    QPixmap pix(size());
    pix.fill(Qt::transparent);
    QPainter p(&pix);
    p.setPen(textCol);
    for (int i=0; i<10; ++i) {
        int barHeight=(lineWidth*constHeightStep)*(i+1);
        QRect r(reverse ? pix.width()-(widthStep+(i*lineWidth*widthStep*2))
                        : i*lineWidth*widthStep*2,
                pix.height()-(barHeight+1), (lineWidth*widthStep)-1, barHeight);
        if (filled) {
            p.fillRect(r.adjusted(1, 1, 0, 0), textCol);
        } else if (lineWidth>1) {
            p.drawRect(r);
            p.drawRect(r.adjusted(1, 1, -1, -1));
        } else {
            p.drawRect(r);
        }
    }
    return pix;
}
