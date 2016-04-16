#ifndef CONFIG_H
#define CONFIG_H

#include "core/utils.h"
#include <QLatin1String>

#define MAKE_VERSION(a, b, c) (((a) << 16) | ((b) << 8) | (c))
#define PACKAGE_NAME  "@CMAKE_PROJECT_NAME@"
#define PACKAGE_NAME_CASE  "@CAMEL_CASE_PROJECT@"
#define ORGANIZATION_NAME "@ORGANIZATION_NAME@"
#define PACKAGE_VERSION MAKE_VERSION(@CPACK_PACKAGE_VERSION_MAJOR@, @CPACK_PACKAGE_VERSION_MINOR@, @CPACK_PACKAGE_VERSION_PATCH@)
#define PACKAGE_STRING  PACKAGE_NAME" @PROJECT_VERSION_FULL@"
#define PACKAGE_VERSION_STRING "@PROJECT_VERSION_WITH_SPIN@"
#define INSTALL_PREFIX "@CMAKE_INSTALL_PREFIX@"
#define SHARE_INSTALL_PREFIX "@SHARE_INSTALL_PREFIX@"
#define ICON_INSTALL_PREFIX "@ICON_INSTALL_PREFIX@"
#define LINUX_LIB_DIR "@LINUX_LIB_DIR@"
#define APP_REV_URL "@APP_REV_URL@"
#cmakedefine ENABLE_QTWIDGETS_UI 
#cmakedefine QT_QTDBUS_FOUND
#cmakedefine QT_MAC_EXTRAS_FOUND 1

#define SYS_CONFIG_DIR Core::Utils::systemDir(QLatin1String("config"))
#define SYS_ICONS_DIR  Core::Utils::systemDir(QLatin1String("icons"))
#define SYS_TRANS_DIR  Core::Utils::systemDir(QLatin1String("translations"))

#endif
