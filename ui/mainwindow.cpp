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

#include "ui/mainwindow.h"
#include "ui/utils.h"
#include "ui/thinsplitter.h"
#include "ui/serverview.h"
#include "ui/rendererview.h"
#include "ui/lyricsview.h"
#include "ui/toolbar.h"
#include "ui/actioncollection.h"
#include "ui/notification.h"
#include "ui/utils.h"
#include "ui/preferencesdialog.h"
#ifdef Q_OS_MAC
#include "ui/windowmanager.h"
#include "mac/osxstyle.h"
#endif
#include "upnp/device.h"
#include "core/configuration.h"
#include "core/notificationmanager.h"
#include "core/debug.h"
#include "config.h"
#if !defined Q_OS_WIN && !defined Q_OS_MAC
#include <QDBusConnection>
#include "madrigaladaptor.h"
#endif
#include <QSplitter>
#include <QVBoxLayout>
#include <QMenuBar>
#include <QMenu>
#include <QMessageBox>
#include <QApplication>

Ui::MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
    , preferences(0)
{
    #if !defined Q_OS_WIN && !defined Q_OS_MAC
    new MadrigalAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/MainWindow", this);
    #endif

    Upnp::Device::setMonoIconCol(Utils::clampColor(palette().windowText().color()));
    ActionCollection::get()->setMainWidget(this);
    QWidget *mainWidget=new QWidget(this);
    QVBoxLayout *mainLayout=new QVBoxLayout(mainWidget);
    Notification *notif=new Notification(this);
    splitter=new ThinSplitter(mainWidget);
    ServerView *server=new ServerView(splitter);
    renderer=new RendererView(splitter);
    lyrics=new LyricsView(splitter);
    toolBar=new ToolBar(this);
    server->setMinimumWidth(Utils::scaleForDpi(340));
    lyrics->setMinimumWidth(Utils::scaleForDpi(300));
    setMinimumWidth(Utils::scaleForDpi(800));

    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(splitter);
    setCentralWidget(mainWidget);
    setWindowTitle(PACKAGE_NAME_CASE);

    Core::Configuration cfg(this);
    resize(cfg.get("size", QSize(Utils::scaleForDpi(640), Utils::scaleForDpi(480))));

    QList<int> s=cfg.get("splitter", QList<int>());
    splitter->setSizes(3==s.count() ? s : QList<int>() << server->minimumWidth() << Utils::scaleForDpi(400) << lyrics->minimumWidth());
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 2);
    splitter->setStretchFactor(2, 1);

    QPoint pos=cfg.get("pos", QPoint());
    if (!pos.isNull()) {
        move(pos);
    }

    toolBar->setLyricsVisible(cfg.get("lyrics", false));
    lyrics->setVisible(toolBar->isLyricsVisible());
    toolBar->adjustSize();
    notif->setOffset(toolBar->height()+Utils::scaleForDpi(32));

    QMenu *mnu=new QMenu(PACKAGE_NAME_CASE, this);
    mnu->addAction(tr("Preferences..."), this, SLOT(showPreferences()))->setMenuRole(QAction::PreferencesRole);
    mnu->addAction(tr("About %1...").arg(PACKAGE_NAME_CASE), this, SLOT(showAbout()))->setMenuRole(QAction::AboutRole);
    mnu->addAction(tr("Quit"), qApp, SLOT(quit()), QKeySequence::Quit)->setMenuRole(QAction::QuitRole);

    #ifdef Q_OS_WIN
    toolBar->addMenuButton(mnu);
    #elif defined Q_OS_MAC
    menuBar()->addMenu(mnu);
    #else
    if (Utils::Unity==Utils::currentDe()) {
        menuBar()->addMenu(mnu);
    } else {
        toolBar->addMenuButton(mnu);
    }
    #endif
    #ifdef Q_OS_MAC
    addToolBar(toolBar);
    WindowManager *wm=new WindowManager(toolBar);
    wm->registerWidgetAndChildren(toolBar);
    toolBar->setMovable(false);
    toolBar->setContextMenuPolicy(Qt::PreventContextMenu);
    setUnifiedTitleAndToolBarOnMac(true);
    // For Mac, dont set a Window Icon - as this is draggable.
    // Set icon on toolbar, and use that as parent for about dialog
    toolBar->setWindowIcon(QIcon::fromTheme(PACKAGE_NAME));
    Mac::OSXStyle::self()->initWindowMenu(this);
    #else
    setWindowIcon(QIcon::fromTheme(PACKAGE_NAME));
    #endif

    connect(new Core::NotificationManager(this), SIGNAL(msg(QString)), notif, SLOT(show(QString)));
    connect(toolBar, SIGNAL(showLyrics(bool)), lyrics, SLOT(setVisible(bool)));
}

Ui::MainWindow::~MainWindow() {
    DBUG(Ui);
    Core::Configuration cfg(this);
    cfg.set("size", size());
    cfg.set("pos", pos());
    QList<int> sizes=splitter->sizes();
    if (3==sizes.count() && 0==sizes.at(2) && !toolBar->isLyricsVisible()) {
        int lw=lyrics->width();
        sizes=QList<int>() << sizes.at(0) << (sizes.at(1)-lw) << lw;
    }
    cfg.set("splitter", sizes);
    cfg.set("lyrics", toolBar->isLyricsVisible());
}

void Ui::MainWindow::raise() {
    DBUG(Ui);
    Ui::Utils::raiseWindow(this);
}

void Ui::MainWindow::showPreferences() {
    if (preferences) {
        Ui::Utils::raiseWindow(preferences);
    } else {
        preferences=new Ui::PreferencesDialog(this, renderer);
        preferences->show();
        connect(preferences, SIGNAL(destroyed(QObject*)), SLOT(preferencesDestroyed()));
    }
}

void Ui::MainWindow::showAbout() {
    #ifdef Q_OS_MAC
    QWidget *p=toolBar;
    #else
    QWidget *p=this;
    #endif
    QMessageBox::about(p, tr("About %1").arg(PACKAGE_NAME_CASE),
                       tr("<b>%1 %2</b><br/><br/>OpenHome control point.<br/><br/>"
                          "&copy; 2016 Craig Drummond<br/>"
                          "Released under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GPLv3</a>").
                       arg(PACKAGE_NAME_CASE).arg(PACKAGE_VERSION_STRING));

}

void Ui::MainWindow::preferencesDestroyed() {
    preferences=0;
}
