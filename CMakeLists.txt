PROJECT(madrigal)
set(CAMEL_CASE_PROJECT Madrigal)

cmake_minimum_required(VERSION 2.8)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/../cmake)

set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md")
set(CPACK_GENERATOR "DEB;RPM;TBZ2")
set(DEBIAN_PACKAGE_DESCRIPTION "UPnP Control Point")
set(CPACK_SOURCE_GENERATOR "TBZ2")
set(CPACK_PACKAGE_VERSION_MAJOR "0")
set(CPACK_PACKAGE_VERSION_MINOR "0")
set(CPACK_PACKAGE_VERSION_PATCH "1")
set(CPACK_PACKAGE_VERSION_SPIN "")  # Use ".$number" - e.g. ".1"
set(CPACK_PACKAGE_CONTACT "Craig Drummond <craig.p.drummond@gmail.com>")
set(PROJECT_VERSION "${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}")
set(PROJECT_VERSION_FULL "${PROJECT_VERSION}.${CPACK_PACKAGE_VERSION_PATCH}")
set(PROJECT_VERSION_WITH_SPIN "${PROJECT_VERSION_FULL}${CPACK_PACKAGE_VERSION_SPIN}")
set(CPACK_SOURCE_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION_WITH_SPIN}")
set(CPACK_PACKAGE_FILE_NAME "${CMAKE_PROJECT_NAME}-${PROJECT_VERSION_WITH_SPIN}")
set(CPACK_SOURCE_IGNORE_FILES  "/.svn/;/.git/;.gitignore;CMakeLists.txt.user")
include(CPack)
set(ORGANIZATION_NAME "craigd")
set(APP_REV_URL "${ORGANIZATION_NAME}.${CMAKE_PROJECT_NAME}")

# TODO: QML ?
set(ENABLE_QTWIDGETS_UI ON)

if (WIN32)
    # TODO
    message("WARNING: This app has not been fully ported to Windows - and probably does not even compile!")
    set(ICON_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}/icons/${CMAKE_PROJECT_NAME})
    set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    install(FILES upnp/mapping DESTINATION ${SHARE_INSTALL_PREFIX}/config)

    add_definitions(-DWIN32)
    add_subdirectory(windows)
    if (NOT APP_WINDOWS_INSTALLER_DEST)
        set(APP_WINDOWS_INSTALLER_DEST z:\ )
    endif (NOT APP_WINDOWS_INSTALLER_DEST)
    if (MINGW)
        # resource compilation for MinGW
        ADD_CUSTOM_COMMAND(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ico.o
                           COMMAND windres.exe -I${CMAKE_CURRENT_SOURCE_DIR} -i${CMAKE_SOURCE_DIR}/windows/appico.rc 
                                               -o ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ico.o)
        set(APP_SRCS ${APP_SRCS} ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}ico.o)
    else (MINGW)
        set(APP_SRCS ${APP_SRCS} windows/appico.rc)
    endif (MINGW)

elseif (APPLE)
    # TODO
    message("WARNING: This app has not been fully ported to OSX - and probably does not even compile!")
    set(MACOSX_BUNDLE_BUNDLE_NAME ${CAMEL_CASE_PROJECT})
    set(MACOSX_BUNDLE_EXECUTABLE ${CMAKE_PROJECT_NAME})
    set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_VERSION ${PROJECT_VERSION})
    set(MACOSX_BUNDLE_LONG_VERSION_STRING ${PROJECT_VERSION_WITH_SPIN})
    set(MACOSX_BUNDLE_ICON_FILE ${CMAKE_PROJECT_NAME}.icns)
    set(APP_CONTENTS_DIR ${CMAKE_INSTALL_PREFIX}/${CAMEL_CASE_PROJECT}.app/Contents)
    set(MACOSX_BUNDLE_RESOURCES ${APP_CONTENTS_DIR}/Resources)
    set(MACOSX_BUNDLE_APP_DIR ${APP_CONTENTS_DIR}/MacOS)
    set(ICON_INSTALL_PREFIX ${MACOSX_BUNDLE_RESOURCES}/icons/${CMAKE_PROJECT_NAME})
    set(SHARE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX})
    install(FILES upnp/mapping DESTINATION ${SHARE_INSTALL_PREFIX}/config)
    set(APP_SRCS ${APP_SRCS} mac/notify.cpp mac/notification.mm)
    set(APP_MOC_HDRS ${APP_MOC_HDRS} mac/notify.h)
else ()
    set(SHARE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/share"
        CACHE PATH "Define install directory for read-only architecture-independent data")
    set(ICON_INSTALL_PREFIX ${SHARE_INSTALL_PREFIX}/icons/hicolor)
    if (HELPERS_LIB_DIR AND CMAKE_SIZEOF_VOID_P EQUAL 8)
        set(LINUX_LIB_DIR ${HELPERS_LIB_DIR})
    else ()
        set(LINUX_LIB_DIR lib)
    endif ()
    install(FILES upnp/mapping DESTINATION ${SHARE_INSTALL_PREFIX}/${CMAKE_PROJECT_NAME}/config)
endif ()

find_package(Qt5Core REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Xml REQUIRED)
find_package(Qt5Svg REQUIRED)

if (Qt5_POSITION_INDEPENDENT_CODE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
endif (Qt5_POSITION_INDEPENDENT_CODE)

add_definitions(${Qt5Network_DEFINITIONS} ${Qt5Xml_DEFINITIONS} ${Qt5Svg_DEFINITIONS})
include_directories(${CMAKE_SOURCE_DIR} ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR}
                    ${Qt5Network_INCLUDE_DIRS} ${Qt5Core_INCLUDE_DIRS} ${Qt5Svg_INCLUDE_DIRS})

set(APP_RCS icons/icons.qrc)
qt5_add_resources(APP_RC_SRCS ${APP_RCS})

set(APP_SRCS ${APP_RC_SRCS}
    core/main.cpp core/debug.cpp core/thread.cpp core/utils.cpp core/networkaccessmanager.cpp
    core/configuration.cpp core/monoicon.cpp core/images.cpp core/actions.cpp core/mediakeys.cpp
    upnp/ssdp.cpp upnp/device.cpp upnp/devicesmodel.cpp upnp/mediaservers.cpp upnp/mediaserver.cpp
    upnp/renderers.cpp upnp/ohrenderer.cpp upnp/httpserver.cpp upnp/httpconnection.cpp
    upnp/model.cpp upnp/renderer.cpp)

set(APP_MOC_HDRS
    core/thread.h core/networkaccessmanager.h core/images.h core/mediakeys.h
    upnp/ssdp.h upnp/device.h upnp/devicesmodel.h upnp/mediaservers.h upnp/mediaserver.h
    upnp/renderers.h upnp/renderer.h upnp/renderer.h upnp/ohrenderer.h upnp/httpserver.h
    upnp/httpconnection.h upnp/model.h)

if (ENABLE_QTWIDGETS_UI)
    if (WIN32 OR APPLE)
        set(APP_LIBRARIES ${APP_LIBRARIES} qtsingleapplication)
        set(APP_SRCS ${APP_SRCS} ui/application_single.cpp)
        set(APP_MOC_HDRS ${APP_MOC_HDRS} ui/application_single.h)
        if (APPLE)
            set(APP_SRCS ${APP_SRCS} mac/osxstyle.cpp ui/windowmanager.cpp ui/flattoolbutton.cpp)
            set(APP_MOC_HDRS ${APP_MOC_HDRS} mac/osxstyle.h ui/windowmanager.h)
        endif()
    else ()
        find_package(Qt5DBus REQUIRED)
        set(QT_QTDBUS_FOUND ON)
        set(APP_LIBRARIES ${APP_LIBRARIES} ${Qt5DBus_LIBRARIES})
        set(APP_SRCS ${APP_SRCS} ui/application_dbus.cpp ui/gtkproxystyle.cpp ui/windowmanager.cpp)
        set(APP_MOC_HDRS ${APP_MOC_HDRS} ui/windowmanager.h)
        add_subdirectory(ui/themes)
    endif()
    find_package(Qt5Widgets REQUIRED)
    add_definitions(${Qt5Widgets_DEFINITIONS})
    include_directories(${Qt5Widgets_INCLUDE_DIRS})
    set(APP_LIBRARIES ${APP_LIBRARIES} ${Qt5Widgets_LIBRARIES})
    if (NOT Qt5_POSITION_INDEPENDENT_CODE) # ???
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Qt5Widgets_EXECUTABLE_COMPILE_FLAGS}")
    endif()
    set(APP_SRCS ${APP_SRCS} ui/mainwindow.cpp ui/utils.cpp ui/basicitemdelegate.cpp ui/gtkstyle.cpp
        ui/proxystyle.cpp ui/serverview.cpp ui/rendererview.cpp ui/thinsplitter.cpp ui/navbutton.cpp
        ui/listview.cpp ui/toolbar.cpp ui/toolbutton.cpp ui/volumeslider.cpp ui/nowplayingwidget.cpp
        ui/squeezedtextlabel.cpp ui/action.cpp ui/actioncollection.cpp ui/viewtoolbar.cpp
        ui/libraryview.cpp ui/listitemdelegate.cpp ui/albuminfo.cpp ui/animatedicon.cpp
        ui/statuslabel.cpp ui/groupeditemdelegate.cpp ui/menubutton.cpp ui/preferencesdialog.cpp)
    set(APP_MOC_HDRS ${APP_MOC_HDRS} ui/mainwindow.h ui/serverview.h ui/rendererview.h
        ui/navbutton.h ui/listview.h ui/volumeslider.h ui/nowplayingwidget.h ui/action.h ui/actioncollection.h
        ui/toolbar.h ui/libraryview.h ui/listitemdelegate.h ui/albuminfo.h ui/animatedicon.h ui/statuslabel.h
        ui/preferencesdialog.h)
endif()

if (QT_QTDBUS_FOUND)
    add_definitions(${Qt5DBus_DEFINITIONS})
    include_directories(${Qt5DBus_INCLUDE_DIRS})
    configure_file(dbus/app.xml.cmake ${CMAKE_BINARY_DIR}/dbus/${APP_REV_URL}.xml)
    qt5_add_dbus_adaptor(APP_SRCS dbus/org.mpris.MediaPlayer2.Player.xml dbus/mpris.h Dbus::Mpris)
    qt5_add_dbus_adaptor(APP_SRCS dbus/org.mpris.MediaPlayer2.root.xml dbus/mpris.h Dbus::Mpris)
    qt5_add_dbus_adaptor(APP_SRCS ${CMAKE_BINARY_DIR}/dbus/${APP_REV_URL}.xml ui/mainwindow.h Ui::MainWindow)
    set_source_files_properties(dbus/org.gnome.SettingsDaemon.xml dbus/org.gnome.SettingsDaemon.MediaKeys.xml PROPERTIES NO_NAMESPACE TRUE)
    qt5_add_dbus_interfaces(APP_SRCS dbus/org.gnome.SettingsDaemon.xml)
    qt5_add_dbus_interfaces(APP_SRCS dbus/org.gnome.SettingsDaemon.MediaKeys.xml)
    qt5_add_dbus_interfaces(APP_SRCS dbus/org.freedesktop.Notifications.xml)
    set(APP_LIBRARIES ${APP_LIBRARIES} ${Qt5DBus_LIBRARIES})
    set(APP_SRCS ${APP_SRCS} dbus/mpris.cpp dbus/gnomemediakeys.cpp dbus/notify.cpp)
    set(APP_MOC_HDRS ${APP_MOC_HDRS} dbus/mpris.h dbus/gnomemediakeys.h dbus/notify.h)
endif ()

configure_file(config.h.cmake ${CMAKE_BINARY_DIR}/config.h)

add_subdirectory(icons)

qt5_wrap_cpp(APP_MOC_SRCS ${APP_MOC_HDRS})
add_executable(${CMAKE_PROJECT_NAME} ${APP_SRCS} ${APP_MOC_SRCS})
target_link_libraries(${PROJECT_NAME} ${Qt5Core_LIBRARIES} ${Qt5Network_LIBRARIES} ${Qt5Xml_LIBRARIES} ${Qt5Svg_LIBRARIES} ${APP_LIBRARIES})

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin LIBRARY DESTINATION lib ARCHIVE DESTINATION lib)

if (UNIX AND NOT APPLE)
    configure_file(app.desktop.cmake ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop)
    install(FILES ${CMAKE_BINARY_DIR}/${PROJECT_NAME}.desktop DESTINATION "${SHARE_INSTALL_PREFIX}/applications")
endif()
