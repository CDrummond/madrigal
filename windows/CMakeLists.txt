set(WINDOWS_URL "http://${PROJECT_URL}")

FILE(TO_CMAKE_PATH $ENV{QTDIR} QT_DIR)
set(QT_BINARY_DIR ${QT_DIR}/bin)
set(QT_PLUGINS_DIR ${QT_DIR}/plugins)
set(QT_TRANSLATIONS_DIR ${QT_DIR}/translations)

configure_file(app.nsi.cmake ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.nsi)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_PROJECT_NAME}.nsi DESTINATION ${CMAKE_INSTALL_PREFIX})
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/app.ico DESTINATION ${CMAKE_INSTALL_PREFIX} RENAME ${CMAKE_PROJECT_NAME}.ico)

# Qt image formats...
install(FILES ${QT_PLUGINS_DIR}/imageformats/qjpeg.dll ${QT_PLUGINS_DIR}/imageformats/qsvg.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/imageformats/)

# Qt iconengines...
install(FILES ${QT_PLUGINS_DIR}/iconengines/qsvgicon.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/iconengines/)

# Qt windows platform plugin...
install(FILES ${QT_PLUGINS_DIR}/platforms/qwindows.dll DESTINATION ${CMAKE_INSTALL_PREFIX}/platforms/)

# Qt, taglib, and zlib libraries...
set(EXTRA_WIN_LIBS ${EXTRA_WIN_LIBS}
    ${QT_BINARY_DIR}/Qt5Core.dll ${QT_BINARY_DIR}/Qt5Gui.dll ${QT_BINARY_DIR}/Qt5Network.dll ${QT_BINARY_DIR}/Qt5Svg.dll ${QT_BINARY_DIR}/Qt5Xml.dll
    ${QT_BINARY_DIR}/Qt5Widgets.dll
    ${CMAKE_CURRENT_SOURCE_DIR}/icu/icudt52.dll ${QT_BINARY_DIR}/icuin52.dll ${QT_BINARY_DIR}/icuuc52.dll
    ${QT_BINARY_DIR}/libgcc_s_dw2-1.dll ${QT_BINARY_DIR}/libstdc++-6.dll ${QT_BINARY_DIR}/libwinpthread-1.dll)

# Qt translation files...
file(GLOB qt_trans ${QT_TRANSLATIONS_DIR}/qt_*.qm)
foreach(qm ${qt_trans})
    if (NOT ${qm} MATCHES "(${QT_TRANSLATIONS_DIR}/qt_help*)")
        list(APPEND qt_translations "${qm}")
    endif (NOT ${qm} MATCHES "(${QT_TRANSLATIONS_DIR}/qt_help*)")
endforeach(qm ${qt_trans})
install(FILES ${qt_translations} DESTINATION ${CMAKE_INSTALL_PREFIX}/translations/)

install(FILES ${EXTRA_WIN_LIBS} DESTINATION ${CMAKE_INSTALL_PREFIX})

