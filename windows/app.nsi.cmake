!define APPNAME "@CAMEL_CASE_PROJECT@"
!define COMPANYNAME "@CAMEL_CASE_PROJECT@"
!define DESCRIPTION "OpenHome Control Point"
!define VERSIONMAJOR @CPACK_PACKAGE_VERSION_MAJOR@
!define VERSIONMINOR @CPACK_PACKAGE_VERSION_MINOR@
!define VERSIONBUILD @CPACK_PACKAGE_VERSION_PATCH@@CPACK_PACKAGE_VERSION_SPIN@
#!define HELPURL "http://..." # "Support Information" link
#!define UPDATEURL "http://..." # "Product Updates" link
!define ABOUTURL "https://github.com/CDrummond/@PROJECT_NAME@" # "Publisher" link
 
RequestExecutionLevel admin

SetCompressor /SOLID lzma
!include "MUI2.nsh"
 
InstallDir "$PROGRAMFILES\@CAMEL_CASE_PROJECT@"
# This will be in the installer/uninstaller's title bar
Name "@CAMEL_CASE_PROJECT@"
Icon "@PROJECT_NAME@.ico"
outFile "@CAMEL_CASE_PROJECT@-@APP_VERSION_WITH_SPIN@-Setup.exe"

!define MUI_ABORTWARNING
!define MUI_ICON "@PROJECT_NAME@.ico"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_LICENSE "LICENSE.txt"
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_UNPAGE_FINISH

!insertmacro MUI_LANGUAGE "English" ;first language is the default language
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"
!insertmacro MUI_LANGUAGE "Spanish"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "SimpChinese"
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "Korean"
!insertmacro MUI_LANGUAGE "Italian"
!insertmacro MUI_LANGUAGE "Dutch"
!insertmacro MUI_LANGUAGE "Danish"
!insertmacro MUI_LANGUAGE "Swedish"
!insertmacro MUI_LANGUAGE "Norwegian"
!insertmacro MUI_LANGUAGE "NorwegianNynorsk"
!insertmacro MUI_LANGUAGE "Finnish"
!insertmacro MUI_LANGUAGE "Greek"
!insertmacro MUI_LANGUAGE "Russian"
!insertmacro MUI_LANGUAGE "Portuguese"
!insertmacro MUI_LANGUAGE "PortugueseBR"
!insertmacro MUI_LANGUAGE "Polish"
!insertmacro MUI_LANGUAGE "Ukrainian"
!insertmacro MUI_LANGUAGE "Czech"
!insertmacro MUI_LANGUAGE "Slovak"
!insertmacro MUI_LANGUAGE "Croatian"
!insertmacro MUI_LANGUAGE "Bulgarian"
!insertmacro MUI_LANGUAGE "Hungarian"
!insertmacro MUI_LANGUAGE "Thai"
!insertmacro MUI_LANGUAGE "Romanian"
!insertmacro MUI_LANGUAGE "Latvian"
!insertmacro MUI_LANGUAGE "Macedonian"
!insertmacro MUI_LANGUAGE "Estonian"
!insertmacro MUI_LANGUAGE "Turkish"
!insertmacro MUI_LANGUAGE "Lithuanian"
!insertmacro MUI_LANGUAGE "Slovenian"
!insertmacro MUI_LANGUAGE "Serbian"
!insertmacro MUI_LANGUAGE "SerbianLatin"
!insertmacro MUI_LANGUAGE "Arabic"
!insertmacro MUI_LANGUAGE "Farsi"
!insertmacro MUI_LANGUAGE "Hebrew"
!insertmacro MUI_LANGUAGE "Indonesian"
!insertmacro MUI_LANGUAGE "Mongolian"
!insertmacro MUI_LANGUAGE "Luxembourgish"
!insertmacro MUI_LANGUAGE "Albanian"
!insertmacro MUI_LANGUAGE "Breton"
!insertmacro MUI_LANGUAGE "Belarusian"
!insertmacro MUI_LANGUAGE "Icelandic"
!insertmacro MUI_LANGUAGE "Malay"
!insertmacro MUI_LANGUAGE "Bosnian"
!insertmacro MUI_LANGUAGE "Kurdish"
!insertmacro MUI_LANGUAGE "Irish"
!insertmacro MUI_LANGUAGE "Uzbek"
!insertmacro MUI_LANGUAGE "Galician"
!insertmacro MUI_LANGUAGE "Afrikaans"
!insertmacro MUI_LANGUAGE "Catalan"
!insertmacro MUI_LANGUAGE "Esperanto"

section "install"
    # Files for the install directory - to build the installer, these should be in the same directory as the install script (this file)
    setOutPath $INSTDIR
    # Files added here should be removed by the uninstaller (see section "uninstall")
    file "@PROJECT_NAME@.exe"
    file "Qt5Core.dll"
    file "Qt5Gui.dll"
    file "Qt5Network.dll"
    file "Qt5Svg.dll"
    file "Qt5Widgets.dll"
    file "icudt52.dll"
    file "icuin52.dll"
    file "icuuc52.dll"
    file "libgcc_s_dw2-1.dll"
    file "libstdc++-6.dll"
    file "libwinpthread-1.dll"
    @APP_SSL_WIN_NSIS_INSTALL@
    setOutPath $INSTDIR\config
    file "config\mapping"
    setOutPath $INSTDIR\iconengines
    file "iconengines\qsvgicon.dll"
    setOutPath $INSTDIR\platforms
    file "platforms\qwindows.dll"
    setOutPath $INSTDIR\icons
    setOutPath $INSTDIR\icons\@PROJECT_NAME@
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\64
    file "icons\@PROJECT_NAME@\64\@PROJECT_NAME@.png"
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\48
    file "icons\@PROJECT_NAME@\48\@PROJECT_NAME@.png"
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\32
    file "icons\@PROJECT_NAME@\32\@PROJECT_NAME@.png"
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\22
    file "icons\@PROJECT_NAME@\22\@PROJECT_NAME@.png"
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\16
    file "icons\@PROJECT_NAME@\16\@PROJECT_NAME@.png"
    setOutPath $INSTDIR\icons\@PROJECT_NAME@\svg
    @APP_PROXY_ICON_INSTALL@

    setOutPath $INSTDIR\imageformats
    file "imageformats\qjpeg.dll"
    file "imageformats\qsvg.dll"
    setOutPath $INSTDIR\translations
    file "translations\APP_cs.qm"
    file "translations\APP_de.qm"
    file "translations\APP_en_GB.qm"
    file "translations\APP_es.qm"
    file "translations\APP_fr.qm"
    file "translations\APP_hu.qm"
    file "translations\APP_ko.qm"
    file "translations\APP_pl.qm"
    file "translations\APP_ru.qm"
    file "translations\APP_zh_CN.qm"

    file "translations\qt_ar.qm"
    file "translations\qt_cs.qm"
    file "translations\qt_da.qm"
    file "translations\qt_de.qm"
    file "translations\qt_es.qm"
    file "translations\qt_fa.qm"
    file "translations\qt_fi.qm"
    file "translations\qt_fr.qm"
    file "translations\qt_gl.qm"
    file "translations\qt_he.qm"
    file "translations\qt_hu.qm"
    file "translations\qt_it.qm"
    file "translations\qt_ja.qm"
    file "translations\qt_ko.qm"
    file "translations\qt_lt.qm"
    file "translations\qt_pl.qm"
    file "translations\qt_pt.qm"
    file "translations\qt_ru.qm"
    file "translations\qt_sk.qm"
    file "translations\qt_sl.qm"
    file "translations\qt_sv.qm"
    file "translations\qt_uk.qm"
    file "translations\qt_zh_CN.qm"
    file "translations\qt_zh_TW.qm"
 
    writeUninstaller "$INSTDIR\uninstall.exe"
 
    # Start Menu
    createShortCut "$SMPROGRAMS\@CAMEL_CASE_PROJECT@.lnk" "$INSTDIR\@PROJECT_NAME@.exe" "" "$INSTDIR\@PROJECT_NAME@.exe"
 
    # Registry information for add/remove programs
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "DisplayName" "@CAMEL_CASE_PROJECT@"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "UninstallString" "$\"$INSTDIR\uninstall.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "QuietUninstallString" "$\"$INSTDIR\uninstall.exe$\" /S"
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "InstallLocation" "$\"$INSTDIR$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "DisplayIcon" "$\"$INSTDIR\@PROJECT_NAME@.exe$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "Publisher" "@CAMEL_CASE_PROJECT@"
#    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "HelpLink" "$\"${HELPURL}$\""
#    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "URLUpdateInfo" "$\"${UPDATEURL}$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "URLInfoAbout" "$\"@WINDOWS_URL@$\""
    WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "DisplayVersion" "@APP_VERSION_WITH_SPIN@"
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "VersionMajor" @CPACK_PACKAGE_VERSION_MAJOR@
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "VersionMinor" @CPACK_PACKAGE_VERSION_MINOR@
    # There is no option for modifying or repairing the install
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "NoModify" 1
    WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "NoRepair" 1
    # Set the INSTALLSIZE constant (!defined at the top of this script) so Add/Remove Programs can accurately report the size
    # WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@" "EstimatedSize" ${INSTALLSIZE}
sectionEnd
 
# Uninstaller
 
section "uninstall"
    # Remove Start Menu launcher
    delete "$SMPROGRAMS\@CAMEL_CASE_PROJECT@.lnk"
 
    delete "$INSTDIR\@PROJECT_NAME@.exe"
    delete "$INSTDIR\@PROJECT_NAME@ README.txt"
    delete "$INSTDIR\@PROJECT_NAME@ License (GPL V3).txt"

    delete "$INSTDIR\iconengines\qsvgicon.dll"
    delete "$INSTDIR\icons\@PROJECT_NAME@\index.theme"
    delete "$INSTDIR\icons\@PROJECT_NAME@\64\@PROJECT_NAME@.png"
    delete "$INSTDIR\icons\@PROJECT_NAME@\48\@PROJECT_NAME@.png"
    delete "$INSTDIR\icons\@PROJECT_NAME@\32\@PROJECT_NAME@.png"
    delete "$INSTDIR\icons\@PROJECT_NAME@\22\@PROJECT_NAME@.png"
    delete "$INSTDIR\icons\@PROJECT_NAME@\16\@PROJECT_NAME@.png"
    delete "$INSTDIR\icons\@PROJECT_NAME@\svg\@PROJECT_NAME@.svg"

    delete "$INSTDIR\imageformats\qjpeg4.dll"
    delete "$INSTDIR\imageformats\qsvg4.dll"
    delete "$INSTDIR\imageformats\qjpeg.dll"
    delete "$INSTDIR\imageformats\qsvg.dll"
    delete "$INSTDIR\platforms\qwindows.dll"

    delete "$INSTDIR\libgcc_s_dw2-1.dll"
    delete "$INSTDIR\mingwm10.dll"

    delete "$INSTDIR\Qt5Core.dll"
    delete "$INSTDIR\Qt5Gui.dll"
    delete "$INSTDIR\Qt5Network.dll"
    delete "$INSTDIR\Qt5Svg.dll"
    delete "$INSTDIR\Qt5Widgets.dll"

    delete "$INSTDIR\icudt52.dll"
    delete "$INSTDIR\icuin52.dll"
    delete "$INSTDIR\icuuc52.dll"
    delete "$INSTDIR\libgcc_s_dw2-1.dll"
    delete "$INSTDIR\libstdc++-6.dll"
    delete "$INSTDIR\libwinpthread-1.dll"

    delete "$INSTDIR\translations\APP_cs.qm"
    delete "$INSTDIR\translations\APP_de.qm"
    delete "$INSTDIR\translations\APP_en_GB.qm"
    delete "$INSTDIR\translations\APP_es.qm"
    delete "$INSTDIR\translations\APP_fr.qm"
    delete "$INSTDIR\translations\APP_hu.qm"
    delete "$INSTDIR\translations\APP_ko.qm"
    delete "$INSTDIR\translations\APP_pl.qm"
    delete "$INSTDIR\translations\APP_ru.qm"
    delete "$INSTDIR\translations\APP_zh_CN.qm"
    delete "$INSTDIR\translations\qt_ar.qm"
    delete "$INSTDIR\translations\qt_cs.qm"
    delete "$INSTDIR\translations\qt_da.qm"
    delete "$INSTDIR\translations\qt_de.qm"
    delete "$INSTDIR\translations\qt_es.qm"
    delete "$INSTDIR\translations\qt_fa.qm"
    delete "$INSTDIR\translations\qt_fi.qm"
    delete "$INSTDIR\translations\qt_fr.qm"
    delete "$INSTDIR\translations\qt_gl.qm"
    delete "$INSTDIR\translations\qt_he.qm"
    delete "$INSTDIR\translations\qt_hu.qm"
    delete "$INSTDIR\translations\qt_it.qm"
    delete "$INSTDIR\translations\qt_ja.qm"
    delete "$INSTDIR\translations\qt_ko.qm"
    delete "$INSTDIR\translations\qt_lt.qm"
    delete "$INSTDIR\translations\qt_pl.qm"
    delete "$INSTDIR\translations\qt_pt.qm"
    delete "$INSTDIR\translations\qt_ru.qm"
    delete "$INSTDIR\translations\qt_sk.qm"
    delete "$INSTDIR\translations\qt_sl.qm"
    delete "$INSTDIR\translations\qt_sv.qm"
    delete "$INSTDIR\translations\qt_uk.qm"
    delete "$INSTDIR\translations\qt_zh_CN.qm"
    delete "$INSTDIR\translations\qt_zh_TW.qm"

    delete "$INSTDIR\config\mapping"
    rmDir $INSTDIR\config

    rmDir $INSTDIR\icons\@PROJECT_NAME@\128
    rmDir $INSTDIR\icons\@PROJECT_NAME@\64
    rmDir $INSTDIR\icons\@PROJECT_NAME@\48
    rmDir $INSTDIR\icons\@PROJECT_NAME@\32
    rmDir $INSTDIR\icons\@PROJECT_NAME@\22
    rmDir $INSTDIR\icons\@PROJECT_NAME@\16
    rmDir $INSTDIR\icons\@PROJECT_NAME@\svg
    rmDir $INSTDIR\icons\@PROJECT_NAME@\svg64
    rmDir $INSTDIR\icons\@PROJECT_NAME@

    rmDir $INSTDIR\icons
    rmDir $INSTDIR\imageformats
    rmDir $INSTDIR\platforms
    rmDir $INSTDIR\translations

    # Always delete uninstaller as the last action
    delete $INSTDIR\uninstall.exe
 
    # Try to remove the install directory - this will only happen if it is empty
    rmDir $INSTDIR
 
    # Remove uninstaller information from the registry
    DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\@CAMEL_CASE_PROJECT@ @CAMEL_CASE_PROJECT@"
sectionEnd
