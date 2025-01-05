# SPDX-FileCopyrightText: 2025 Laurent Montel <montel@kde.org>
#
# SPDX-License-Identifier: CC0-1.0

add_executable(${PROJECT_NAME} WIN32)
target_sources(${PROJECT_NAME} PRIVATE main.cpp)
target_link_libraries(${PROJECT_NAME} Kaidan::Core)

ecm_qt_declare_logging_category(
    ${PROJECT_NAME}
    HEADER kaidan_debug.h
    IDENTIFIER KAIDAN_LOG
    CATEGORY_NAME im.kaidan.kaidan
    DESCRIPTION "Kaidan"
    EXPORT KAIDAN
)

target_sources(${PROJECT_NAME} PRIVATE ../qml/qml.qrc)

if (TARGET KF6::Crash)
    target_link_libraries(${PROJECT_NAME} KF6::Crash)
endif()

# Set a custom plist file for the app bundle
if(APPLE)
    if(IOS)
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/ios/Info.plist)
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/macos/Info.plist)
    endif()
endif()

