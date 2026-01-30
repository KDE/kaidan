# SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

cmake_minimum_required(VERSION 3.16)
set(CMAKE_CXX_STANDARD 23)
set(QT_MIN_VERSION "6.7.0")
set(KF_MIN_VERSION "6.11.0")

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
kde_enable_exceptions()

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    add_compile_options("-stdlib=libc++")
endif()

# application information
set(APPLICATION_ID "im.kaidan.kaidan")
set(APPLICATION_NAME "kaidan")
set(APPLICATION_DISPLAY_NAME "Kaidan")
set(APPLICATION_DESCRIPTION "User-friendly and modern chat app for every device")

# Version
set(VERSION_MAJOR 0)
set(VERSION_MINOR 15)
set(VERSION_PATCH 0)
set(VERSION_CODE 26)
set(VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# CMake options
option(USE_KNOTIFICATIONS "Use KNotifications for displaying notifications" TRUE)
option(BUNDLE_ICONS "Bundle breeze icons" FALSE)

set(AUTOMOC_MOC_OPTIONS -Muri=${APPLICATION_ID})

# Find packages
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED NO_MODULE COMPONENTS Core Concurrent Qml Quick Svg Sql QuickControls2 Xml Multimedia Positioning Location)

if(Qt6Gui_VERSION VERSION_GREATER_EQUAL "6.10.0" AND NOT WIN32 AND NOT APPLE)
    find_package(Qt6GuiPrivate ${QT_MIN_VERSION} REQUIRED NO_MODULE)
endif()

find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS KIO Kirigami Prison)
if (WIN32)
    find_package(KF6 ${KF_MIN_VERSION} REQUIRED COMPONENTS IconThemes)
endif()
if (NOT ANDROID)
   find_package(KF6 ${KF_MIN_VERSION} OPTIONAL_COMPONENTS Crash)
   find_package(KDSingleApplication-qt6 CONFIG REQUIRED)
   find_package(KF6WindowSystem ${KF_MIN_VERSION} CONFIG REQUIRED)
endif()

find_package(KF6KirigamiAddons 1.8.0 REQUIRED)
find_package(QXmppQt6 1.14.0 REQUIRED COMPONENTS Omemo)

find_package(ICU 61.0 COMPONENTS uc)
set_package_properties(ICU PROPERTIES
    TYPE REQUIRED
    PURPOSE "Unicode library (used for emoji support)"
)

find_package(Qt6Keychain 0.15 REQUIRED)
set_package_properties(Qt6Keychain PROPERTIES
    TYPE REQUIRED
    DESCRIPTION "Secure credential storage"
    URL "https://github.com/frankosterfeld/qtkeychain"
)

pkg_check_modules(GStreamer REQUIRED IMPORTED_TARGET gstreamer-1.0>=1.20)

ecm_set_disabled_deprecation_versions(
    QT 6.8.0
    KF 6.12.0
)

# Format
include(KDEClangFormat)
file(GLOB_RECURSE ALL_CLANG_FORMAT_SOURCE_FILES *.cpp *.h *.hpp *.c)
kde_clang_format(${ALL_CLANG_FORMAT_SOURCE_FILES})

include(KDEGitCommitHooks)
kde_configure_git_pre_commit_hook(CHECKS CLANG_FORMAT)

# QML modules
ecm_find_qmlmodule(org.kde.kquickimageeditor 1.0)

if(USE_KNOTIFICATIONS)
    find_package(KF6Notifications ${KF_MIN_VERSION} REQUIRED)
    set(__KF6Notifications_LIBRARIES KF6::Notifications)
    add_definitions(-DHAVE_KNOTIFICATIONS)
endif()

# Optional QWidget style integration (not on mobile)
if(NOT ANDROID AND NOT IOS)
    find_package(Qt6Widgets)
    find_package(KF6QQC2DesktopStyle)
    set_package_properties(Qt6Widgets PROPERTIES
        DESCRIPTION "Integration with QWidget based desktop styles"
        TYPE OPTIONAL
    )
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

if(UNIX AND NOT APPLE)
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

if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

ecm_qt_install_logging_categories(
     EXPORT KAIDAN
     FILE kaidan.categories
     DESTINATION ${KDE_INSTALL_LOGGINGCATEGORIESDIR}
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
