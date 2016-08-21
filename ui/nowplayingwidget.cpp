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

#include "ui/nowplayingwidget.h"
#include "ui/squeezedtextlabel.h"
#include "ui/utils.h"
#include "core/utils.h"
#include "core/configuration.h"
#ifdef Q_OS_MAC
#include "mac/osxstyle.h"
#endif
#include <QGridLayout>
#include <QProxyStyle>
#include <QApplication>
#include <QMouseEvent>
#include <QToolTip>
#include <QSpacerItem>

class PosSliderProxyStyle : public QProxyStyle {
public:
    PosSliderProxyStyle()
        : QProxyStyle()
    {
        setBaseStyle(qApp->style());
    }

    int styleHint(StyleHint stylehint, const QStyleOption *opt, const QWidget *widget, QStyleHintReturn *returnData) const {
        if (QStyle::SH_Slider_AbsoluteSetButtons==stylehint) {
            return Qt::LeftButton|QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        } else {
            return QProxyStyle::styleHint(stylehint, opt, widget, returnData);
        }
    }
};

Ui::TimeLabel::TimeLabel(QWidget *p, QSlider *s)
    : QLabel(p)
    , slider(s)
    , pressed(false)
    , showRemaining(false)
{
    setAttribute(Qt::WA_Hover, true);
    setAlignment((isRightToLeft() ? Qt::AlignLeft : Qt::AlignRight)|Qt::AlignVCenter);
    // For some reason setting this here does not work!
    // setStyleSheet(QLatin1String("QLabel:hover {color:palette(highlight);}"));

    showRemaining=Core::Configuration(metaObject()->className()).get("showRemaining", false);
}

Ui::TimeLabel::~TimeLabel() {
    Core::Configuration(metaObject()->className()).set("showRemaining", showRemaining);
}

void Ui::TimeLabel::setRange(int min, int max) {
    QLabel::setEnabled(min!=max);
    if (!isEnabled()) {
        setText(QLatin1String(" "));
    }
}

void Ui::TimeLabel::updateTime() {
    if (isEnabled()) {
        int value=showRemaining ? slider->maximum()-slider->value() : slider->maximum();
        QString prefix=showRemaining && value ? QLatin1String("-") : QString();
        if (isRightToLeft()) {
            setText(QString("%1 / %2").arg(prefix+Core::Utils::formatTime(value), Core::Utils::formatTime(slider->value())));
        } else {
            setText(QString("%1 / %2").arg(Core::Utils::formatTime(slider->value()), prefix+Core::Utils::formatTime(value)));
        }
    } else {
        setText(QLatin1String(" "));
    }
}

bool Ui::TimeLabel::event(QEvent *e) {
    switch (e->type()) {
    case QEvent::MouseButtonPress:
        if (isEnabled() && Qt::NoModifier==static_cast<QMouseEvent *>(e)->modifiers() && Qt::LeftButton==static_cast<QMouseEvent *>(e)->button()) {
            pressed=true;
        }
        break;
    case QEvent::MouseButtonRelease:
        if (isEnabled() && pressed) {
            showRemaining=!showRemaining;
            updateTime();
        }
        pressed=false;
        break;
    case QEvent::HoverEnter:
        if (isEnabled()) {
            #ifdef Q_OS_MAC
            setStyleSheet(QString("QLabel{color:%1;}").arg(Mac::OSXStyle::self()->viewPalette().highlight().color().name()));
            #else
            setStyleSheet(QLatin1String("QLabel{color:palette(highlight);}"));
            #endif
        }
        break;
    case QEvent::HoverLeave:
        if (isEnabled()) {
            setStyleSheet(QString());
        }
    default:
        break;
    }
    return QLabel::event(e);
}

Ui::PosSlider::PosSlider(QWidget *p)
    : QSlider(p)
{
    setPageStep(0);
    setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    setFocusPolicy(Qt::NoFocus);
    setStyle(new PosSliderProxyStyle());
    int h=qMax((int)(fontMetrics().height()*0.5), 8);
    setMinimumHeight(h);
    setMaximumHeight(h);
    updateStyleSheet();
    setMouseTracking(true);
}

void Ui::PosSlider::updateStyleSheet() {
    int lineWidth=maximumHeight()>12 ? 2 : 1;

    QString boderFormat=QLatin1String("QSlider::groove:horizontal { border: %1px solid rgba(%2, %3, %4, %5); "
                                      "background: solid rgba(%2, %3, %4, %6); "
                                      "border-radius: %7px } ");
    QString fillFormat=QLatin1String("QSlider::")+QLatin1String(isRightToLeft() ? "add" : "sub")+
                       QLatin1String("-page:horizontal {border: %1px solid rgb(%3, %4, %5); "
                                     "background: solid rgb(%3, %4, %5); "
                                     "border-radius: %1px; margin: %2px} ")+
                       QLatin1String("QSlider::")+QLatin1String(isRightToLeft() ? "add" : "sub")+
                       QLatin1String("-page:horizontal:disabled {border: 0px; background: solid rgba(0, 0, 0, 0)}");
    QLabel lbl(parentWidget());
    lbl.ensurePolished();
    QColor textColor=lbl.palette().color(QPalette::Active, QPalette::Text);
    #ifdef Q_OS_MAC
    QColor fillColor=Mac::OSXStyle::self()->viewPalette().highlight().color();
    #else
    QColor fillColor=lbl.palette().highlight().color();
    #endif
    int alpha=textColor.value()<32 ? 96 : 64;

    setStyleSheet(boderFormat.arg(lineWidth).arg(textColor.red()).arg(textColor.green()).arg(textColor.blue()).arg(alpha)
                             .arg(alpha/4).arg(lineWidth*2)+
                  fillFormat.arg(lineWidth).arg(lineWidth*2).arg(fillColor.red()).arg(fillColor.green()).arg(fillColor.blue()));
}

void Ui::PosSlider::mouseMoveEvent(QMouseEvent *e) {
    if (maximum()!=minimum()) {
        qreal pc = (qreal)e->pos().x()/(qreal)width();
        QPoint pos(e->pos().x(), height());
        QToolTip::showText(mapToGlobal(pos), Core::Utils::formatTime(maximum()*pc), this, rect());
    }

    QSlider::mouseMoveEvent(e);
}

void Ui::PosSlider::wheelEvent(QWheelEvent *ev) {
    if (!isEnabled()) {
        return;
    }

    static const int constStep=5;
    int numDegrees = ev->delta() / 8;
    int numSteps = numDegrees / 15;
    int val=value();
    if (numSteps > 0) {
        int max=maximum();
        if (val!=max) {
            for (int i = 0; i < numSteps; ++i) {
                val+=constStep;
                if (val>max) {
                    val=max;
                    break;
                }
            }
        }
    } else {
        int min=minimum();
        if (val!=min) {
            for (int i = 0; i > numSteps; --i) {
                val-=constStep;
                if (val<min) {
                    val=min;
                    break;
                }
            }
        }
    }
    if (val!=value()) {
        setValue(val);
        emit positionSet();
    }
}

void Ui::PosSlider::setRange(int min, int max) {
    bool active=min!=max;
    QSlider::setRange(min, max);
    setValue(min);
    if (!active) {
        setToolTip(QString());
    }

    setEnabled(active);
}

Ui::NowPlayingWidget::NowPlayingWidget(QWidget *p)
    : QWidget(p)
    , shown(false)
{
    cover=new QLabel(this);
    track=new SqueezedTextLabel(this);
    artist=new SqueezedTextLabel(this);
    slider=new PosSlider(this);
    time=new TimeLabel(this, slider);
    QFont f=track->font();
    QFont small=Utils::smallFont(f);
    f.setBold(true);
    track->setFont(f);
    artist->setFont(small);
    time->setFont(small);
    int coverSize=Utils::scaleForDpi(56);
    cover->setFixedSize(QSize(coverSize, coverSize));
    slider->setOrientation(Qt::Horizontal);
    QGridLayout *layout=new QGridLayout(this);
    int space=Utils::layoutSpacing(this);
    int pad=Utils::scaleForDpi(2);
    layout->setContentsMargins(pad, pad, pad, pad);
    layout->setSpacing((space/2)+pad);
    layout->addWidget(cover, 0, 0, 5, 1);
    layout->addItem(new QSpacerItem(space, 1, QSizePolicy::Fixed, QSizePolicy::Fixed), 0, 1, 1, 1);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding), 0, 2, 1, 1);
    layout->addWidget(track, 1, 2, 1, 3);
    layout->addWidget(artist, 2, 2, 1, 1);
    layout->addWidget(time, 2, 3, 1, 1);
    layout->addWidget(slider, 3, 2, 1, 2);
    layout->addItem(new QSpacerItem(0, 0, QSizePolicy::Fixed, QSizePolicy::MinimumExpanding), 4, 2, 1, 1);
    connect(slider, SIGNAL(sliderReleased()), this, SLOT(sliderReleased()));
    connect(slider, SIGNAL(positionSet()), this, SLOT(sliderReleased()));
    connect(slider, SIGNAL(valueChanged(int)), time, SLOT(updateTime()));
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    clearTimes();
    update(QModelIndex());
    connect(Core::Images::self(), SIGNAL(found(Core::ImageDetails)), this, SLOT(coverLoaded(Core::ImageDetails)));
}

void Ui::NowPlayingWidget::update(const QModelIndex &idx) {
    const Upnp::Device::MusicTrack *song=idx.isValid() ? static_cast<const Upnp::Device::MusicTrack *>(idx.internalPointer()) : 0;
    setEnabled(0!=song);
    if (!song) {
        track->setText(" ");
        artist->setText(" ");
        updatePos(0);
        updateDuration(0);
        updateCover(0);
        slider->setEnabled(false);
    } else {
        track->setText(song->name);
        artist->setText(song->artistAndAlbum());
        if (artist->text().isEmpty() && song->isBroadcast) {
            artist->setText("<i><small>"+tr("Stream")+"</small></i>");
        }
        slider->setEnabled(!song->isBroadcast);
        Core::ImageDetails cover=song->cover();
        if (currentCover.artist!=cover.artist || currentCover.album!=cover.album) {
            updateCover(&cover);
        }
    }
}

void Ui::NowPlayingWidget::updatePos(quint32 val) {
    slider->setValue(val);
    time->updateTime();
}

void Ui::NowPlayingWidget::updateDuration(quint32 val) {
    slider->setRange(0, val);
    time->setRange(0, val);
    time->updateTime();
}

void Ui::NowPlayingWidget::clearTimes() {
    slider->setRange(0, 0);
    time->setRange(0, 0);
    time->updateTime();
}

int Ui::NowPlayingWidget::value() const {
    return slider->value();
}

void Ui::NowPlayingWidget::showEvent(QShowEvent *e) {
    QWidget::showEvent(e);
    if (!shown) {
        slider->updateStyleSheet();
        shown=true;
    }
}

void Ui::NowPlayingWidget::updateCover(const Core::ImageDetails *cvr) {
    QImage *img=0;
    if (cvr) {
        currentCover=*cvr;
        img=Core::Images::self()->get(currentCover, cover->height());
    } else {
        currentCover.album=QString();
        currentCover.artist=QString();
        img=Core::Images::self()->get(Core::ImageDetails(), cover->height(), true);
    }

    if (img) {
        cover->setPixmap(QPixmap::fromImage(*img));
    } else {
        cover->setPixmap(QPixmap());
    }
}

void Ui::NowPlayingWidget::sliderReleased() {
    emit seek(slider->value());
}

void Ui::NowPlayingWidget::coverLoaded(const Core::ImageDetails &image) {
    if (image.artist==currentCover.artist && image.album==currentCover.album) {
        QImage *img=Core::Images::self()->get(image, cover->height(), true);
        if (img) {
            cover->setPixmap(QPixmap::fromImage(*img));
        }
    }
}
