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

#include "config.h"
#include "core/debug.h"
#include "core/thread.h"
#include "upnp/model.h"

#ifdef ENABLE_QTWIDGETS_UI

#if defined Q_OS_WIN || defined Q_OS_MAC
#include "ui/application_single.h"
#else
#include "ui/application_dbus.h"
#endif
#define APP_CLASS Ui::Application

#ifdef Q_OS_MAC
#include "mac/notify.h"
#endif

#include "ui/mainwindow.h"

#else // ENABLE_QTWIDGETS_UI
#include <QCoreApplication>
#define APP_CLASS QCoreApplication
#endif // ENABLE_QTWIDGETS_UI

#ifdef QT_QTDBUS_FOUND
#include "dbus/mpris.h"
#include "dbus/gnomemediakeys.h"
#include "dbus/notify.h"
#endif

#include <QCommandLineParser>
#include <QThread>

int main(int argc, char *argv[]) {
    QCoreApplication::setApplicationName(PACKAGE_NAME);
    QCoreApplication::setOrganizationName(ORGANIZATION_NAME);
    QCoreApplication::setApplicationVersion(PACKAGE_VERSION_STRING);
    APP_CLASS app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr("OpenHome Control Point"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption debugOption(QStringList() << "d" << "debug", "Set debug areas", "debug", "");
    parser.addOption(debugOption);
    parser.process(app);
    if (parser.isSet(debugOption)) {
        Core::Debug::setAreas(parser.value(debugOption));
    }

    if (!app.start()) {
        return 0;
    }

    QThread::currentThread()->setObjectName("Main");
    Core::ThreadCleaner::self();
    Upnp::Model::self();

    #ifdef ENABLE_QTWIDGETS_UI
    Ui::MainWindow mw(0);

    #if defined Q_OS_WIN || defined Q_OS_MAC
    app.setActivationWindow(&mw);
    #endif

    mw.show();

    #else

    // TODO: QML UI !!!!

    #endif // ENABLE_QTWIDGETS_UI

    #ifdef QT_QTDBUS_FOUND
    Dbus::Mpris *mpris=new Dbus::Mpris(&app);
    QObject::connect(mpris, SIGNAL(showMainWindow()), &mw, SLOT(raise()));
    Dbus::GnomeMediaKeys *mk=new Dbus::GnomeMediaKeys(&app);
    Dbus::Notify::self();
    mk->activate();
    #endif

    #ifdef Q_OS_MAC
    Mac::Notify::self();
    #endif

    return app.exec();
}

