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

#include "ui/application_dbus.h"
#include "ui/utils.h"
#include "config.h"
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDir>
#include <QIcon>

Ui::Application::Application(int &argc, char **argv)
    : QApplication(argc, argv)
{
//    setAttribute(Qt::AA_UseHighDpiPixmaps);
}

Ui::Application::~Application() {
}

bool Ui::Application::start() {
    if (QDBusConnection::sessionBus().registerService(APP_REV_URL)) {
        QIcon::setThemeSearchPaths(QStringList() << SHARE_INSTALL_PREFIX"/icons" << QIcon::themeSearchPaths());
//        if (Ui::Utils::KDE!=Ui::Utils::currentDe() || QLatin1String("breeze")==QIcon::themeName()) {
//            QIcon::setThemeSearchPaths(QStringList() << SYS_ICONS_DIR << QIcon::themeSearchPaths());
//            QIcon::setThemeName(PACKAGE_NAME);
//            if (Ui::Utils::KDE!=Ui::Utils::currentDe()) {
//                setAttribute(Qt::AA_DontShowIconsInMenus, true);
//            }
//        }
        if (Ui::Utils::KDE!=Ui::Utils::currentDe()) {
            setAttribute(Qt::AA_DontShowIconsInMenus, true);
        }
        return true;
    }
    QDBusConnection::sessionBus().send(QDBusMessage::createMethodCall(APP_REV_URL, "/MainWindow", "", "raise"));
    return false;
}

