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

#include "ui/rendererview.h"
#include "ui/listview.h"
#include "ui/flattoolbutton.h"
#include "ui/listitemdelegate.h"
#include "ui/groupeditemdelegate.h"
#include "ui/viewtoolbar.h"
#include "ui/flattoolbutton.h"
#include "ui/squeezedtextlabel.h"
#include "ui/utils.h"
#include "ui/actioncollection.h"
#include "ui/action.h"
#include "upnp/model.h"
#include "upnp/renderers.h"
#include "upnp/renderer.h"
#include "upnp/device.h"
#include "core/debug.h"
#include "core/configuration.h"
#include "core/monoicon.h"
#include "core/utils.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QMenu>

enum Pages {
    Page_Info,
    Page_Renderers,
    Page_Renderer
};

Ui::RendererView::RendererView(QWidget *p)
    : QWidget(p)
{
    QVBoxLayout *mainLayout=new QVBoxLayout(this);
    toolbar=new ViewToolBar(this);
    toolbar->setTitle(tr("Outputs"));
    stack=new QStackedWidget(this);
    model=Upnp::Model::self()->renderersModel();
    proxy=new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    QFrame *info=new QFrame(stack);
    QVBoxLayout *infoLayout=new QVBoxLayout(info);
    infoLabel=new QLabel(info);
    cancelButton=new QPushButton(info);
    cancelButton->setText(tr("Use first output found"));
    infoLayout->addWidget(infoLabel);
    infoLayout->addItem(new QSpacerItem(0, Utils::scaleForDpi(32)));
    infoLayout->addWidget(cancelButton);
    infoLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    infoLabel->setWordWrap(true);
    stack->addWidget(info);
    QWidget *view=new QWidget(stack);
    QGridLayout *viewLayout=new QGridLayout(view);
    queue=new ListView(view);
    queue->setDragDropMode(QAbstractItemView::DragDrop);
    renderers=new ListView(stack);
    stack->addWidget(renderers);
    rendererSelect=new FlatToolButton(toolbar);
    rendererSelect->setToolTip(tr("Select Output"));
    rendererSelect->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    toolbar->addWidget(rendererSelect);
    viewLayout->addWidget(queue);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);
    stack->addWidget(view);
    stack->setCurrentIndex(Page_Info);
    renderers->setModel(proxy);
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(updateItems()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(updateItems()));
    connect(model, SIGNAL(activeDevice(QModelIndex)), SLOT(setActive(QModelIndex)));
    connect(cancelButton, SIGNAL(clicked(bool)), SLOT(useFirst()));
    connect(renderers, SIGNAL(clicked(QModelIndex)), SLOT(rendererSelected(QModelIndex)));
    connect(rendererSelect, SIGNAL(clicked(bool)), SLOT(selectRenderer()));
    setInfoLabel();
    queue->setItemDelegate(new GroupedItemDelegate(queue));
    queue->setUniformItemSizes(false);
    renderers->setItemDelegate(new ListItemDelegate(renderers));
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(stack);
    setMinimumWidth(Utils::scaleForDpi(450));

    QColor red(220, 0, 0);
    iconColor=Utils::clampColor(palette().foreground().color());
    repeatAction=ActionCollection::get()->createAction("repeat", tr("Repeat"), Core::MonoIcon::icon(Core::MonoIcon::refresh, iconColor, iconColor));
    shuffleAction=ActionCollection::get()->createAction("random", tr("Random"), Core::MonoIcon::icon(Core::MonoIcon::random, iconColor, iconColor));
    clearAction=ActionCollection::get()->createAction("clear", tr("Clear"), Core::MonoIcon::icon(Core::MonoIcon::timescircle, red, red));
    removeAction=ActionCollection::get()->createAction("remove", tr("Remove Selected Tracks"), Core::MonoIcon::icon(Core::MonoIcon::scissors, red, red));
    clearAction->setShortcut(Qt::ControlModifier+Qt::Key_K);
    removeAction->setShortcut(Qt::ControlModifier+Qt::Key_X);
    repeatAction->setShortcut(Qt::ControlModifier+Qt::Key_R);
    shuffleAction->setShortcut(Qt::ControlModifier+Qt::Key_U);

    repeatAction->setCheckable(true);
    shuffleAction->setCheckable(true);
    FlatToolButton *repeatButton=new FlatToolButton(toolbar);
    FlatToolButton *shuffleButton=new FlatToolButton(toolbar);
    FlatToolButton *removeButton=new FlatToolButton(toolbar);
    FlatToolButton *clearButton=new FlatToolButton(toolbar);
    repeatButton->setDefaultAction(repeatAction);
    shuffleButton->setDefaultAction(shuffleAction);
    removeButton->setDefaultAction(removeAction);
    clearButton->setDefaultAction(clearAction);
    queueInfo=new SqueezedTextLabel(toolbar);
    queueInfo->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
    queueInfo->setTextElideMode(Qt::ElideLeft);
    toolbar->addWidget(queueInfo, false);
    toolbar->addSpacer(Utils::layoutSpacing(this), false);
    toolbar->addWidget(repeatButton, false);
    toolbar->addWidget(shuffleButton, false);
    toolbar->addSpacer(Utils::layoutSpacing(this), false);
    toolbar->addWidget(removeButton, false);
    toolbar->addWidget(clearButton, false);
    backIcon=Core::MonoIcon::icon(Qt::LeftToRight==layoutDirection() ? Core::MonoIcon::chevronleft : Core::MonoIcon::chevronright, iconColor, iconColor);
    connect(Core::Images::self(), SIGNAL(found(Core::ImageDetails)), queue, SLOT(coverFound(Core::ImageDetails)));
    updateStats(0, 0);
    removeAction->setEnabled(false);
    connect(queue, SIGNAL(itemsSelected(bool)), removeAction, SLOT(setEnabled(bool)));
    connect(removeAction, SIGNAL(triggered(bool)), this, SLOT(removeSelectedTracks()));
    connect(clearAction, SIGNAL(triggered(bool)), this, SLOT(clearQueue()));
    autoScrollQueue=Core::Configuration(this).get("scroll", true);
}

Ui::RendererView::~RendererView() {
}

void Ui::RendererView::setScrollQueue(bool s) {
    if (s!=autoScrollQueue) {
        autoScrollQueue=s;
        Core::Configuration(this).set("scroll", autoScrollQueue);
        if (autoScrollQueue) {
            Upnp::Renderer *renderer=(Upnp::Renderer *)queue->model();
            if (renderer) {
                scrollTo(renderer->current());
            }
        }
    }
}

void Ui::RendererView::setInfoLabel() {
    infoLabel->setText("<i>"+(model->rowCount()>0
                                ? tr("Waiting for previous output...")
                                : tr("Looking for outputs..."))+"</i>");
    cancelButton->setVisible(model->rowCount()>0);
}

void Ui::RendererView::updateItems(bool activeSet) {
    DBUG(Ui) << (void *)sender() << activeSet << model->isInitialising() << stack->currentIndex();
    if (model->isInitialising()) {
        setInfoLabel();
    } else if (activeSet || Page_Renderers!=stack->currentIndex()) {
        stack->setCurrentIndex(model->rowCount()<1 ? Page_Info : Page_Renderer);
        toolbar->showTitle(model->rowCount()<1);
        if (model->rowCount()<1) {
            setInfoLabel();
        }
        if (!queue->model() && renderers->model()->rowCount()>0) {
            rendererSelected(proxy->index(0, 0));
        }
    }
}

void Ui::RendererView::setActive(const QModelIndex &idx) {
    DBUG(Ui) << (void *)sender() << idx.isValid();
    if (queue->model()) {
        disconnect(queue->model(), SIGNAL(queueDetails(quint32,quint32)), this, SLOT(updateStats(quint32,quint32)));
        disconnect(queue->model(), SIGNAL(repeat(bool)), repeatAction, SLOT(setChecked(bool)));
        disconnect(queue->model(), SIGNAL(shuffle(bool)), shuffleAction, SLOT(setChecked(bool)));
        disconnect(queue->model(), SIGNAL(currentTrack(QModelIndex)), this, SLOT(scrollTo(QModelIndex)));
        disconnect(repeatAction, SIGNAL(toggled(bool)), queue->model(), SLOT(setRepeat(bool)));
        disconnect(shuffleAction, SIGNAL(toggled(bool)), queue->model(), SLOT(setShuffle(bool)));
        disconnect(queue, SIGNAL(doubleClicked(QModelIndex)), queue->model(), SLOT(play(QModelIndex)));
        disconnect(queue, SIGNAL(activated(QModelIndex)), queue->model(), SLOT(play(QModelIndex)));
        updateStats(0, 0);
        removeAction->setEnabled(false);
        queue->setModel(0);
    }
    if (idx.isValid()) {
        Upnp::Renderer *renderer=static_cast<Upnp::Renderer *>(idx.internalPointer());
        queue->setModel(renderer);
        connect(renderer, SIGNAL(queueDetails(quint32,quint32)), this, SLOT(updateStats(quint32,quint32)));
        connect(renderer, SIGNAL(repeat(bool)), repeatAction, SLOT(setChecked(bool)));
        connect(renderer, SIGNAL(shuffle(bool)), shuffleAction, SLOT(setChecked(bool)));
        connect(renderer, SIGNAL(currentTrack(QModelIndex)), this, SLOT(scrollTo(QModelIndex)));
        connect(repeatAction, SIGNAL(toggled(bool)), renderer, SLOT(setRepeat(bool)));
        connect(shuffleAction, SIGNAL(toggled(bool)), renderer, SLOT(setShuffle(bool)));
        connect(queue, SIGNAL(doubleClicked(QModelIndex)), renderer, SLOT(play(QModelIndex)));
        connect(queue, SIGNAL(activated(QModelIndex)), renderer, SLOT(play(QModelIndex)));
        updateItems();
        rendererSelect->setText(idx.data().toString());
        rendererSelect->setIcon(Core::MonoIcon::icon(renderer->icon(), iconColor, iconColor));
        removeAction->setEnabled(!queue->selectedIndexes().isEmpty());
        repeatAction->setChecked(renderer->playback().repeat);
        shuffleAction->setChecked(renderer->playback().shuffle);
    }
}

void Ui::RendererView::rendererSelected(const QModelIndex &idx) {
    DBUG(Ui) << (void *)sender() << idx.isValid();
    if (idx.isValid()) {
        // idx comes from servers view, which is using proxy (for sorting) - therefore need to map to source
        QModelIndex mapped=proxy->mapToSource(idx);
        if (mapped.isValid() && mapped.internalPointer() && static_cast<Upnp::Device *>(mapped.internalPointer())->isActive()) {
            // Selected previously active device, so just set ui back to this
            setActive(mapped);
        } else {
            // New device, let model control...
            model->setActive(mapped.row());
        }
        updateItems(true);
    }
}

void Ui::RendererView::useFirst() {
    DBUG(Ui);
    rendererSelected(proxy->index(0, 0));
}

void Ui::RendererView::selectRenderer() {
    DBUG(Ui);
    queue->setRootIndex(QModelIndex());
    stack->setCurrentIndex(Page_Renderers);
    toolbar->showTitle(true);
}

void Ui::RendererView::updateStats(quint32 num, quint32 dur) {
    DBUG(Ui) << num << dur;
    if (0==num) {
        queueInfo->setText(tr("No Tracks"));
    } else if (0==dur) {
        if (1==num) {
            queueInfo->setText(tr("1 Track"));
        } else {
            queueInfo->setText(tr("%1 Tracks").arg(num));
        }
    } else {
        if (1==num) {
            queueInfo->setText(tr("1 Track (%1)").arg(Core::Utils::formatTime(dur)));
        } else {
            queueInfo->setText(tr("%1 Tracks (%2)").arg(num).arg(Core::Utils::formatTime(dur)));
        }
    }
    clearAction->setEnabled(num>0);
    removeAction->setEnabled(queue->haveSelectedItems());
}

void Ui::RendererView::clearQueue() {
    if (queue->model() && queue->model()->rowCount()>0 &&
        QMessageBox::Yes==QMessageBox::question(this, tr("Clear Play Queue"), tr("Remove all tracks from play queue?"), QMessageBox::Yes|QMessageBox::No)) {
        Upnp::Renderer *renderer=(Upnp::Renderer *)queue->model();
        if (renderer) {
            renderer->clearQueue();
        }
    }
}

void Ui::RendererView::removeSelectedTracks() {
    Upnp::Renderer *renderer=(Upnp::Renderer *)queue->model();
    if (renderer) {
        renderer->removeTracks(queue->selectedIndexes());
    }
}

void Ui::RendererView::scrollTo(const QModelIndex &idx) {
    if (idx.isValid() && autoScrollQueue) {
        queue->scrollTo(idx, QAbstractItemView::PositionAtCenter);
    }
}
