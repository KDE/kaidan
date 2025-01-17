# SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)
set(QT_MIN_VERSION "6.6.0")
set(KF_MIN_VERSION "6.3.0")

if(POLICY CMP0071)
    cmake_policy(SET CMP0071 NEW)
endif()

project(kaidan)
find_package(ECM ${KF_MIN_VERSION} CONFIG REQUIRED)
# CMake module path
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${ECM_MODULE_PATH})

# CMake modules include
include(KDEInstallDirs)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(KDECMakeSettings)
include(ECMDeprecationSettings)
include(ECMFindQmlModule)
include(ECMInstallIcons)
include(FeatureSummary)
include(ECMQtDeclareLoggingCategory)
include(QmlFormat.cmake)

kde_enable_exceptions()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options("-stdlib=libc++")
endif()

# application information
set(APPLICATION_ID "im.kaidan.kaidan")
set(APPLICATION_NAME "kaidan")
set(APPLICATION_DISPLAY_NAME "Kaidan")
set(APPLICATION_DESCRIPTION "User-friendly and modern chat app for every device")
if(UBUNTU_TOUCH)
    set(APPLICATION_NAME "${APPLICATION_ID}")
endif()

# Version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 11)
set(VERSION_PATCH 0)
set(VERSION_CODE 19)
set(VERSION_EXTRA "" CACHE STRING "Stuff to append to version string")

set(DEVELOPMENT_BUILD TRUE)
if(DEVELOPMENT_BUILD)
    set(VERSION_EXTRA "${VERSION_EXTRA}-dev")
endif()

set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")
if(VERSION_EXTRA)
    set(VERSION_STRING ${VERSION_STRING}${VERSION_EXTRA})
endif()

# CMake options
option(BUILD_TESTS "Build unit tests" FALSE)
option(APPIMAGE "Build Kaidan as AppImage (will only work in the appimage script)" FALSE)
option(UBUNTU_TOUCH "Building an Ubuntu Touch click (internal use only!)" FALSE)
option(CLICK_ARCH "Architecture that will be used in the click's manifest")
option(CLICK_DATE "Date used in the version number in the click's manifest")
option(QUICK_COMPILER "Use QtQuick compiler to improve performance" FALSE)
option(USE_KNOTIFICATIONS "Use KNotifications for displaying notifications" TRUE)
option(BUNDLE_ICONS "Bundle breeze icons" FALSE)

set(AUTOMOC_MOC_OPTIONS -Muri=${APPLICATION_ID})

# Find packages
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Concurrent Qml Quick Svg Sql QuickControls2 Xml Multimedia Positioning Location)
find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS KIO CoreAddons Kirigami)
if (NOT ANDROID)
   find_package(KF6 ${KF_MIN_VERSION} OPTIONAL_COMPONENTS Crash)
endif()

find_package(KF6KirigamiAddons 1.4.0 REQUIRED)
find_package(KF6Prison REQUIRED)
find_package(QXmppQt6 1.9.0 REQUIRED COMPONENTS Omemo)

find_package(ICU 61.0 COMPONENTS uc)
set_package_properties(ICU PROPERTIES
    TYPE REQUIRED
    PURPOSE "Unicode library (used for emoji support)"
)

ecm_set_disabled_deprecation_versions(
    QT 6.3.0
    KF 6.10.0
)

# Format
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.hpp *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

file(GLOB_RECURSE ALL_QML_FORMAT_SOURCE_FILES src/qml/*.qml)
qml_format(${ALL_QML_FORMAT_SOURCE_FILES})

include(KDEGitCommitHooks)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

# QML modules
ecm_find_qmlmodule(org.kde.kquickimageeditor 1.0)

# Optional QtQuickCompiler
if(QUICK_COMPILER)
    find_package(Qt6QuickCompiler)
    set_package_properties(Qt6QuickCompiler PROPERTIES
        DESCRIPTION "Compile QML at build time"
        TYPE OPTIONAL
    )
endif()

if(USE_KNOTIFICATIONS)
    find_package(KF6Notifications REQUIRED)
    set(__KF6Notifications_LIBRARIES KF6::Notifications)
    add_definitions(-DHAVE_KNOTIFICATIONS)
endif()

# Optional QWidget style integration (not on mobile)
if(NOT UBUNTU_TOUCH AND NOT ANDROID AND NOT IOS)
    find_package(Qt6Widgets)
    find_package(KF6QQC2DesktopStyle)
    set_package_properties(Qt6Widgets PROPERTIES
        DESCRIPTION "Integration with QWidget based desktop styles"
        TYPE OPTIONAL
    )
endif()

# Platform-specific
if(Qt6Widgets_FOUND)
    add_definitions(-DHAVE_QWIDGETS -DQAPPLICATION_CLASS=QApplication)
    set(__Qt6Widgets_LIBRARIES Qt6::Widgets)
else()
    add_definitions(-DQAPPLICATION_CLASS=QGuiApplication)
endif()

if(ANDROID)
    find_package(Qt6 REQUIRED COMPONENTS AndroidExtras)
    # We need to link against sqlite3 to help androiddeployqt detect that we need it
    find_package(SQLite3 REQUIRED)

    find_package(OpenSSL 1.1 REQUIRED COMPONENTS Crypto)
    message(STATUS "Using OpenSSL ${OPENSSL_VERSION}")
endif()

include(ECMPoQmTools)

ecm_install_po_files_as_qm(poqm)

#
# Kaidan binary
#

configure_file(${CMAKE_SOURCE_DIR}/GlobalsGen.h.in GlobalsGen.h)

add_subdirectory(src)

#
# Install Kaidan
#

if(ANDROID)
    configure_file(${CMAKE_SOURCE_DIR}/misc/android/AndroidManifest.xml.in
                   ${CMAKE_SOURCE_DIR}/misc/android/AndroidManifest.xml)
elseif(IOS)
    configure_file(${CMAKE_SOURCE_DIR}/misc/ios/Info.plist.in
                   ${CMAKE_SOURCE_DIR}/misc/ios/Info.plist)
endif()

if(UBUNTU_TOUCH AND CLICK_ARCH)
    set(CLICK_VERSION ${VERSION_STRING})
    if(DEVELOPMENT_BUILD)
        set(CLICK_VERSION "${CLICK_VERSION}.${CLICK_DATE}")
    endif()

    # will replace ${CLICK_ARCH} with its value
    configure_file(${CMAKE_SOURCE_DIR}/misc/ubuntu-touch/manifest.json.in
                   ${CMAKE_SOURCE_DIR}/misc/ubuntu-touch/manifest.json)

    # install kaidan binary
    install(PROGRAMS ${CMAKE_CURRENT_BINARY_DIR}/bin/${PROJECT_NAME}
            DESTINATION ${CMAKE_INSTALL_PREFIX}
    )
    # install kaidan media
    install(DIRECTORY "data/"
            DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${APPLICATION_ID}"
            PATTERN "*.qrc" EXCLUDE
    )
    # install icon
    install(FILES "data/images/kaidan.svg" "misc/ubuntu-touch/apparmor.json"
                  "misc/ubuntu-touch/kaidan.desktop" "misc/ubuntu-touch/manifest.json"
            DESTINATION "${CMAKE_INSTALL_PREFIX}"
    )
elseif(UNIX AND NOT APPLE)
    # install kaidan media
    install(DIRECTORY "data/"
            DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/${PROJECT_NAME}"
            PATTERN "*.qrc" EXCLUDE
    )

    # install icon (scalable + 128x)
    install(FILES "data/images/kaidan.svg"
            DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/scalable/apps"
    )
    install(FILES "misc/kaidan-128x128.png"
            DESTINATION "${CMAKE_INSTALL_FULL_DATADIR}/icons/hicolor/128x128/apps"
            RENAME "kaidan.png"
    )
    # install desktop file
    install(FILES "misc/im.kaidan.kaidan.desktop"
            DESTINATION "${KDE_INSTALL_APPDIR}"
    )
    # install metainfo
    install(FILES "misc/im.kaidan.kaidan.appdata.xml"
            DESTINATION "${KDE_INSTALL_METAINFODIR}"
            RENAME "${APPLICATION_ID}.appdata.xml"
    )
endif()

# install kaidan binary
install(TARGETS kaidan
        ${KDE_INSTALL_TARGETS_DEFAULT_ARGS}
)

# KNotifications
if(USE_KNOTIFICATIONS)
    install(FILES misc/kaidan.notifyrc DESTINATION ${KDE_INSTALL_KNOTIFYRCDIR})
endif()

# Dev editable important files, to be seen in the IDE tree view

file(GLOB_RECURSE OTHERS_FILES
    RELATIVE
    ${CMAKE_SOURCE_DIR}
    "utils/*.sh"
    "misc/*.desktop"
    "misc/*.appdata.xml"
    "misc/*.notifyrc"
    "misc/*.in"
    "misc/*.json"
    "*.md"
)

add_custom_target(Others
    SOURCES
    ${OTHERS_FILES}
)

#
# Unit tests
#

if(BUILD_TESTS)
    add_subdirectory(tests)
endif()

ecm_qt_install_logging_categories(
     EXPORT KAIDAN
     FILE kaidan.categories
     DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
