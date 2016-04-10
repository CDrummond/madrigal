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

#include "ui/serverview.h"
#include "ui/listview.h"
#include "ui/navbutton.h"
#include "ui/libraryview.h"
#include "ui/proxystyle.h"
#include "ui/viewtoolbar.h"
#include "ui/utils.h"
#include "ui/listitemdelegate.h"
#include "ui/albuminfo.h"
#include "ui/actioncollection.h"
#include "ui/action.h"
#include "ui/animatedicon.h"
#include "upnp/model.h"
#include "upnp/mediaservers.h"
#include "upnp/mediaserver.h"
#include "core/debug.h"
#include "core/configuration.h"
#include "core/monoicon.h"
#include "core/images.h"
#include "core/actions.h"
#include <QGridLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QSortFilterProxyModel>
#include <QLabel>
#include <QPushButton>
#include <QLineEdit>
#include <QTimer>

enum Pages {
    Page_Info,
    Page_Servers,
    Page_Media
};

Ui::ServerView::ServerView(QWidget *p)
    : QWidget(p)
    , searchTimer(0)
{
    QVBoxLayout *mainLayout=new QVBoxLayout(this);
    toolbar=new ViewToolBar(this);
    toolbar->setTitle(tr("Media Servers"));
    stack=new QStackedWidget(this);
    model=Upnp::Model::self()->serversModel();
    proxy=new QSortFilterProxyModel(this);
    proxy->setSourceModel(model);
    QFrame *info=new QFrame(stack);
    QVBoxLayout *infoLayout=new QVBoxLayout(info);
    infoLabel=new QLabel(info);
    info->setFrameStyle(QFrame::Panel|QFrame::Sunken);
    cancelButton=new QPushButton(info);
    cancelButton->setText(tr("Use first server found"));
    infoLayout->addWidget(infoLabel);
    infoLayout->addItem(new QSpacerItem(0, 32));
    infoLayout->addWidget(cancelButton);
    infoLayout->addItem(new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding));
    infoLabel->setWordWrap(true);
    stack->addWidget(info);
    QWidget *view=new QWidget(stack);
    QGridLayout *viewLayout=new QGridLayout(view);
    albumInfo=new AlbumInfo(view);
    searchText=new QLineEdit(view);
    media=new LibraryView(view);
    servers=new ListView(stack);
    stack->addWidget(servers);
    nav=new NavButton(toolbar);
    nav->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Preferred);
    toolbar->addWidget(nav);
    viewLayout->addWidget(albumInfo);
    viewLayout->addWidget(searchText);
    viewLayout->addWidget(media);
    viewLayout->setMargin(0);
    viewLayout->setSpacing(0);
    stack->addWidget(view);
    stack->setCurrentIndex(Page_Info);
    servers->setModel(proxy);
    connect(model, SIGNAL(rowsInserted(QModelIndex,int,int)), SLOT(updateItems()));
    connect(model, SIGNAL(rowsRemoved(QModelIndex,int,int)), SLOT(updateItems()));
    connect(model, SIGNAL(activeDevice(QModelIndex)), SLOT(setActive(QModelIndex)));
    connect(cancelButton, SIGNAL(pressed()), SLOT(useFirst()));
    connect(media, SIGNAL(itemClicked(QModelIndex)), SLOT(itemClicked(QModelIndex)));
    connect(media, SIGNAL(doAction(int)), SLOT(doAction(int)));
    connect(servers, SIGNAL(clicked(QModelIndex)), SLOT(serverSelected(QModelIndex)));
    connect(nav, SIGNAL(selected(int)), SLOT(navSelected(int)));
    connect(nav, SIGNAL(selected(QModelIndex)), SLOT(navSelected(QModelIndex)));
    connect(nav, SIGNAL(clicked(bool)), SLOT(goBack()));
    connect(albumInfo, SIGNAL(add()), this, SLOT(addAlbum()));
    connect(albumInfo, SIGNAL(play()), this, SLOT(playAlbum()));
    nav->add(tr("Select Library..."), -1);
    setInfoLabel();
    servers->setItemDelegate(new ListItemDelegate(servers));
    info->setProperty(Ui::ProxyStyle::constModifyFrameProp, ProxyStyle::VF_Side);
    media->setProperty(Ui::ProxyStyle::constModifyFrameProp, ProxyStyle::VF_Side);
    servers->setProperty(Ui::ProxyStyle::constModifyFrameProp, ProxyStyle::VF_Side);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(stack);
    connect(Core::Images::self(), SIGNAL(found(Core::ImageDetails)), media, SLOT(coverFound(Core::ImageDetails)));

    spinner=new AnimatedIcon(this);
    spinner->setIcon(Core::MonoIcon::spinner);
    spinner->setIconSize(ViewToolBar::iconSize());
    spinner->setVisible(false);
    toolbar->addWidget(spinner, false);

    searchButton=new FlatToolButton(toolbar);
    QColor col=Utils::clampColor(palette().foreground().color());
    searchAction=ActionCollection::get()->createAction("search", "Search", Core::MonoIcon::icon(Core::MonoIcon::search, col, col));
    searchAction->setShortcut(Qt::ControlModifier+Qt::Key_F);
    searchButton->setDefaultAction(searchAction);
    toolbar->addWidget(searchButton, false);
    searchButton->setVisible(false);
    searchText->setVisible(false);
    searchText->setClearButtonEnabled(true);
    setMinimumWidth(300);
    backIcon=Core::MonoIcon::icon(Qt::LeftToRight==layoutDirection() ? Core::MonoIcon::chevronleft : Core::MonoIcon::chevronright, col, col);
    homeIcon=Core::MonoIcon::icon(Core::MonoIcon::server, col, col);
    Core::Actions::setColor(col);
    connect(searchAction, SIGNAL(triggered(bool)), SLOT(toggleSearch()));
    connect(searchText, SIGNAL(textChanged(QString)), SLOT(startSearchTimer()));
    connect(searchText, SIGNAL(returnPressed()), SLOT(doSearch()));

    Action *backAction=ActionCollection::get()->createAction("goback", tr("Go Back"), backIcon);
    backAction->setShortcut(Qt::ControlModifier+Qt::Key_B);
    nav->setToolTip(backAction->toolTip());
    nav->addAction(backAction);
    connect(backAction, SIGNAL(triggered(bool)), SLOT(goBack()));
}

Ui::ServerView::~ServerView() {
}

void Ui::ServerView::setInfoLabel() {
    infoLabel->setText("<i>"+(model->rowCount()>0
                                ? tr("Waiting for previous media server...")
                                : tr("Looking for media servers..."))+"</i>");
    cancelButton->setVisible(model->rowCount()>0);
}

void Ui::ServerView::showButtons() {
    QModelIndex idx=Page_Media==stack->currentIndex() && media->model() ? media->rootIndex() : QModelIndex();
    searchButton->setVisible(!idx.isValid());
    if (!searchButton->isVisible()) {
        searchText->setVisible(false);
    }
}

void Ui::ServerView::updateItems() {
    if (model->isInitialising()) {
        setInfoLabel();
    } else {
        stack->setCurrentIndex(model->rowCount()<1 ? Page_Info : Page_Media);
        toolbar->showTitle(model->rowCount()<1);
        if (model->rowCount()<1) {
            setInfoLabel();
        }
        if (!media->model() && servers->model()->rowCount()>0) {
            serverSelected(proxy->index(0, 0));
        }
        showButtons();
    }
}

void Ui::ServerView::setActive(const QModelIndex &idx) {
    nav->clear();
    if (media->model()) {
        disconnect(media->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateView(QModelIndex)));
        disconnect(media->model(), SIGNAL(searching(bool)), this, SLOT(searching(bool)));
        disconnect(media->model(), SIGNAL(info(QString,int)), this, SIGNAL(info(QString,int)));
        emit info(QString(),0);
    }
    if (idx.isValid()) {
        media->setModel((QAbstractItemModel *)idx.internalPointer());
        updateItems();
        nav->add(idx.data().toString(), QModelIndex(), homeIcon);
        if (media->model()) {
            connect(media->model(), SIGNAL(rowsInserted(QModelIndex,int,int)), this, SLOT(updateView(QModelIndex)));
            connect(media->model(), SIGNAL(rowsAboutToBeRemoved(QModelIndex,int,int)), this, SLOT(aboutToRemove(QModelIndex,int,int)));
            connect(media->model(), SIGNAL(searching(bool)), this, SLOT(searching(bool)));
            connect(media->model(), SIGNAL(info(QString,int)), this, SIGNAL(info(QString,int)));
        }
    } else {
        media->setModel(0);
    }
    showButtons();
}

void Ui::ServerView::serverSelected(const QModelIndex &idx) {
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
        showButtons();
    }
}

void Ui::ServerView::useFirst() {
    serverSelected(proxy->index(0, 0));
}

void Ui::ServerView::itemClicked(const QModelIndex &idx) {
    if (idx.isValid() && !static_cast<Upnp::Device::Item *>(idx.internalPointer())->isCollection()) {
        return;
    }
    setMediaIndex(idx);
    nav->add(idx, backIcon);
    showButtons();
}

void Ui::ServerView::doAction(int act) {
    if (media->model()) {
        Upnp::MediaServer *server=static_cast<Upnp::MediaServer *>(media->model());
        if (!server->hasCommand()) {
            server->play(media->selectedIndexes(), -1,
                         Core::Actions::Action_Add==act ? Upnp::MediaServer::PlayCommand::Append
                                                        : Upnp::MediaServer::PlayCommand::ReplaceAndPlay);
        }
        // TODO: Else show error...
    }
}

void Ui::ServerView::navSelected(int id) {
    if (-1==id) {
        nav->clear();
        setMediaIndex(QModelIndex());
        stack->setCurrentIndex(Page_Servers);
        toolbar->showTitle(true);
        showButtons();
    }
}

void Ui::ServerView::navSelected(const QModelIndex &idx) {
    nav->removeFrom(idx);
    setMediaIndex(idx);
    showButtons();
}

void Ui::ServerView::goBack() {
    if (QModelIndex()!=media->rootIndex()) {
        navSelected(media->rootIndex().parent());
    } else {
//        nav->showMenu();
        navSelected(-1);
    }
}

void Ui::ServerView::updateView(const QModelIndex &idx, bool force) {
    if (force || idx==media->rootIndex()) {
        media->setIconMode(media->model() && idx.isValid() && media->model()->rowCount(idx)>0 &&
                           Upnp::MediaServer::Collection::Type_Album==static_cast<Upnp::Device::Item *>(idx.child(0,0).internalPointer())->type());
        albumInfo->setVisible(media->model() && idx.isValid() &&
                              Upnp::MediaServer::Collection::Type_Album==static_cast<Upnp::Device::Item *>(idx.internalPointer())->type());
        if (albumInfo->isVisible()) {
            albumInfo->update(static_cast<Upnp::MediaServer::Album *>(idx.internalPointer()));
        } else {
            albumInfo->update(0);
        }
    }
}

void Ui::ServerView::aboutToRemove(const QModelIndex &idx, int from, int to) {
    QModelIndex root=media->rootIndex();
    if (idx==root.parent()) {
        int start=qMin(from, to);
        int end=qMax(from, to);

        for (int r=start; r<=end; ++r) {
            if (r==root.row()) {
                goBack();
                break;
            }
        }
    }
}

void Ui::ServerView::addAlbum() {
    addAlbumToQueue(false);
}

void Ui::ServerView::playAlbum() {
    addAlbumToQueue(true);
}

void Ui::ServerView::toggleSearch() {
    if (searchButton->isVisible()) {
        bool wasVisible=searchText->isVisible();
        searchText->setVisible(!searchText->isVisible());
        if (searchText->isVisible()) {
            if (!wasVisible) {
                searchText->blockSignals(true);
                searchText->setText(QString());
                searchText->blockSignals(false);
            }
            searchText->setFocus();
        }
    }
}

void Ui::ServerView::startSearchTimer() {
    if (searchText->text().isEmpty()) {
        if (searchTimer) {
            searchTimer->stop();
        }
    } else {
        if (!searchTimer) {
            searchTimer=new QTimer(this);
            searchTimer->setSingleShot(true);
            connect(searchTimer, SIGNAL(timeout()), this, SLOT(doSearch()));
        }
        searchTimer->start(1000);
    }
}

void Ui::ServerView::doSearch() {
    QString text=searchText->text().trimmed();

    if (!text.isEmpty() && media->model()) {
        Upnp::MediaServer *server=static_cast<Upnp::MediaServer *>(media->model());
        server->search(text);
        if (searchTimer) {
            searchTimer->stop();
        }
    }
}

void Ui::ServerView::searching(bool status) {
    if (status) {
        Upnp::MediaServer *server=static_cast<Upnp::MediaServer *>(media->model());
        itemClicked(server->searchIndex());
    }
    spinner->setVisible(status);
    spinner->animate(status);
}

void Ui::ServerView::addAlbumToQueue(bool andPlay) {
    if (media->model() && media->rootIndex().isValid()) {
        Upnp::Device::Item *root=static_cast<Upnp::Device::Item *>(media->rootIndex().internalPointer());
        Upnp::MediaServer *server=static_cast<Upnp::MediaServer *>(media->model());
        if (server && Upnp::MediaServer::Collection::Type_Album==root->type()) {
            server->play(QModelIndexList() << media->rootIndex(), -1,
                         andPlay ? Upnp::MediaServer::PlayCommand::ReplaceAndPlay : Upnp::MediaServer::PlayCommand::Append);
        }
    }
}

void Ui::ServerView::setMediaIndex(const QModelIndex &idx) {
    bool navBack=media->rootIndex().isValid() && media->rootIndex().parent()==idx;
    if (navBack) {
        // Update view now, so that scrollTo works!
        updateView(idx, true);
    }
    media->setRootIndex(idx);
    if (!navBack) {
        updateView(idx);
    }
}