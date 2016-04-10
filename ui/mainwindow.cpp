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
#include "ui/toolbar.h"
#include "ui/actioncollection.h"
#include "ui/statuslabel.h"
#include "ui/utils.h"
#include "ui/preferencesdialog.h"
#include "upnp/device.h"
#include "core/configuration.h"
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

Ui::MainWindow::MainWindow(QWidget *p)
    : QMainWindow(p)
{
    #if !defined Q_OS_WIN && !defined Q_OS_MAC
    new MadrigalAdaptor(this);
    QDBusConnection::sessionBus().registerObject("/MainWindow", this);
    #endif

    Upnp::Device::setMonoIconCol(palette().windowText().color());
    ActionCollection::get()->setMainWidget(this);
    QWidget *mainWidget=new QWidget(this);
    QVBoxLayout *mainLayout=new QVBoxLayout(mainWidget);
    msg=new StatusLabel(this);
    splitter=new ThinSplitter(mainWidget);
    ServerView *server=new ServerView(splitter);
    renderer=new RendererView(splitter);
    ToolBar *toolBar=new ToolBar(this);
    mainLayout->setMargin(0);
    mainLayout->setSpacing(0);
    mainLayout->addWidget(toolBar);
    mainLayout->addWidget(msg);
    mainLayout->addWidget(splitter);
    setCentralWidget(mainWidget);
    setWindowTitle(PACKAGE_NAME_CASE);
    Core::Configuration cfg(this);
    resize(cfg.get("size", QSize(640, 480)));
    QByteArray s=cfg.get("splitter", QByteArray());
    if (!s.isEmpty()) {
        splitter->restoreState(s);
    } else {
        splitter->setSizes(QList<int>() << 200 << 600);
    }
    QPoint pos=cfg.get("pos", QPoint());
    if (!pos.isNull()) {
        move(pos);
    }

    connect(server, SIGNAL(info(QString,int)), msg, SLOT(setText(QString,int)));

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
    setWindowIcon(QIcon::fromTheme(PACKAGE_NAME));
}

Ui::MainWindow::~MainWindow() {
    DBUG(Ui);
    Core::Configuration cfg(this);
    cfg.set("size", size());
    cfg.set("pos", pos());
    cfg.set("splitter", splitter->saveState());
}

void Ui::MainWindow::raise() {
    DBUG(Ui);
    Ui::Utils::raiseWindow(this);
}

void Ui::MainWindow::showPreferences() {
    Ui::PreferencesDialog dlg(this, renderer);
    dlg.exec();
}

void Ui::MainWindow::showAbout() {
    QMessageBox::about(this, tr("About %1").arg(PACKAGE_NAME_CASE),
                       tr("<b>%1 %2</b><br/><br/>OpenHome control point.<br/><br/>"
                          "&copy; 2016 Craig Drummond<br/>"
                          "Released under the <a href=\"http://www.gnu.org/licenses/gpl.html\">GPLv3</a>").
                       arg(PACKAGE_NAME_CASE).arg(PACKAGE_VERSION_STRING));

}
