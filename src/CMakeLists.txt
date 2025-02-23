# SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

#
# Sources / Resources
#

add_executable(${PROJECT_NAME} WIN32)

ecm_qt_declare_logging_category(
    ${PROJECT_NAME}
    HEADER kaidan_debug.h
    IDENTIFIER KAIDAN_LOG
    CATEGORY_NAME im.kaidan.kaidan
    DESCRIPTION "kaidan"
    EXPORT KAIDAN
)

target_sources(${PROJECT_NAME} PRIVATE
    AbstractNotifier.h
    AbstractQrCodeGenerator.cpp
    AbstractQrCodeGenerator.h
    Account.cpp
    Account.h
    AccountDb.cpp
    AccountDb.h
    AccountManager.cpp
    AccountManager.h
    AccountMigrationManager.cpp
    AccountMigrationManager.h
    AccountQrCodeGenerator.cpp
    AccountQrCodeGenerator.h
    AccountTrustMessageUriGenerator.cpp
    AccountTrustMessageUriGenerator.h
    Algorithms.h
    AtmManager.cpp
    AtmManager.h
    AuthenticatableEncryptionKeyModel.cpp
    AuthenticatableEncryptionKeyModel.h
    AuthenticatedEncryptionKeyModel.cpp
    AuthenticatedEncryptionKeyModel.h
    AvatarFileStorage.cpp
    AvatarFileStorage.h
    BitsOfBinaryImageProvider.cpp
    BitsOfBinaryImageProvider.h
    Blocking.cpp
    Blocking.h
    ChatController.cpp
    ChatController.h
    ChatHintModel.cpp
    ChatHintModel.h
    ClientWorker.cpp
    ClientWorker.h
    ContactQrCodeGenerator.cpp
    ContactQrCodeGenerator.h
    ContactTrustMessageUriGenerator.cpp
    ContactTrustMessageUriGenerator.h
    CredentialsGenerator.cpp
    CredentialsGenerator.h
    CredentialsValidator.cpp
    CredentialsValidator.h
    DatabaseComponent.cpp
    DatabaseComponent.h
    Database.cpp
    Database.h
    DataFormModel.cpp
    DataFormModel.h
    DiscoveryManager.cpp
    DiscoveryManager.h
    EmojiModel.cpp
    EmojiModel.h
    Encryption.h
    EncryptionController.cpp
    EncryptionController.h
    EncryptionKeyModel.cpp
    EncryptionKeyModel.h
    EncryptionWatcher.cpp
    EncryptionWatcher.h
    Enums.h
    FileModel.cpp
    FileModel.h
    FileProxyModel.cpp
    FileProxyModel.h
    FileProgressCache.cpp
    FileProgressCache.h
    FileSharingController.cpp
    FileSharingController.h
    FutureUtils.h
    Globals.h
    GroupChatController.cpp
    GroupChatController.h
    GroupChatQrCodeGenerator.cpp
    GroupChatQrCodeGenerator.h
    GroupChatUser.cpp
    GroupChatUser.h
    GroupChatUserDb.cpp
    GroupChatUserDb.h
    GroupChatUserFilterModel.cpp
    GroupChatUserFilterModel.h
    GroupChatUserKeyAuthenticationFilterModel.cpp
    GroupChatUserKeyAuthenticationFilterModel.h
    GroupChatUserModel.cpp
    GroupChatUserModel.h
    GuiStyle.h
    HostCompletionModel.cpp
    HostCompletionModel.h
    HostCompletionProxyModel.cpp
    HostCompletionProxyModel.h
    # hsluv-c required for color generation
    hsluv-c/hsluv.c
    hsluv-c/hsluv.h
    JsonUtils.h
    Kaidan.cpp
    Kaidan.h
    LoginQrCodeGenerator.cpp
    LoginQrCodeGenerator.h
    LogHandler.cpp
    LogHandler.h
    main.cpp
    MediaUtils.cpp
    MediaUtils.h
    MessageComposition.cpp
    MessageComposition.h
    Message.cpp
    MessageDb.cpp
    MessageDb.h
    Message.h
    MessageController.cpp
    MessageController.h
    MessageModel.cpp
    MessageModel.h
    MessageReactionModel.cpp
    MessageReactionModel.h
    MixController.cpp
    MixController.h
    Notifications.cpp
    Notifications.h
    OmemoController.cpp
    OmemoController.h
    OmemoDb.cpp
    OmemoDb.h
    PresenceCache.cpp
    PresenceCache.h
    ProviderListItem.cpp
    ProviderListItem.h
    ProviderListModel.cpp
    ProviderListModel.h
    PublicGroupChat.cpp
    PublicGroupChat.h
    PublicGroupChatModel.cpp
    PublicGroupChatModel.h
    PublicGroupChatProxyModel.cpp
    PublicGroupChatProxyModel.h
    PublicGroupChatSearchManager.cpp
    PublicGroupChatSearchManager.h
    QmlUtils.cpp
    QmlUtils.h
    # kaidan QXmpp extensions (need to be merged into QXmpp upstream)
    qxmpp-exts/QXmppColorGenerator.cpp
    qxmpp-exts/QXmppColorGenerator.h
    RegistrationDataFormFilterModel.cpp
    RegistrationDataFormFilterModel.h
    RegistrationDataFormModel.cpp
    RegistrationDataFormModel.h
    RegistrationManager.cpp
    RegistrationManager.h
    RosterDb.cpp
    RosterDb.h
    RosterFilterProxyModel.cpp
    RosterFilterProxyModel.h
    RosterItem.cpp
    RosterItem.h
    RosterItemWatcher.cpp
    RosterItemWatcher.h
    RosterManager.cpp
    RosterManager.h
    RosterModel.cpp
    RosterModel.h
    ServerFeaturesCache.cpp
    ServerFeaturesCache.h
    Settings.cpp
    Settings.h
    SqlUtils.cpp
    SqlUtils.h
    static_plugins.h
    StatusBar.cpp
    StatusBar.h
    SystemUtils.cpp
    SystemUtils.h
    TextFormatter.h
    TextFormatter.cpp
    TrustDb.cpp
    TrustDb.h
    TrustMessageUriGenerator.cpp
    TrustMessageUriGenerator.h
    UserDevicesModel.cpp
    UserDevicesModel.h
    VCardCache.cpp
    VCardCache.h
    VCardManager.cpp
    VCardManager.h
    VCardModel.cpp
    VCardModel.h
    VersionManager.cpp
    VersionManager.h
    XmlUtils.h

    ${CMAKE_SOURCE_DIR}/data/data.qrc
    ${CMAKE_SOURCE_DIR}/misc/misc.qrc
)

if(NOT ANDROID AND NOT IOS)
    target_sources(${PROJECT_NAME} PRIVATE
        singleapp/singleapplication.cpp
        singleapp/singleapplication.h
        singleapp/singleapplication_p.cpp
        singleapp/singleapplication_p.h
    )
endif()

# Include bundled icons on Ubuntu Touch, Android, Windows, macOS and iOS
if(BUNDLE_ICONS OR UBUNTU_TOUCH OR ANDROID)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/kirigami-icons.qrc")
endif()

# Bundled knotifications configuration files on platforms that require it
if(ANDROID)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/misc/notifications.qrc")
endif()

# Bundle images on Android, Windows, macOS and iOS
if(ANDROID OR WIN32 OR APPLE)
    target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/data/images/images.qrc")
endif()

# Set app icon
include(ECMAddAppIcon)
file(GLOB ICONS_SRCS "${CMAKE_SOURCE_DIR}/misc/app-icons/*kaidan.png")
message(STATUS ${ICONS_SRCS})
ecm_add_app_icon(KAIDAN_ICNS ICONS ${ICONS_SRCS})
message(STATUS ${KAIDAN_ICNS})
target_sources(${PROJECT_NAME} PRIVATE ${KAIDAN_ICNS})

target_sources(${PROJECT_NAME} PRIVATE qml/qml.qrc)

ecm_create_qm_loader(${PROJECT_NAME} kaidan_qt)

target_link_libraries(${PROJECT_NAME}
    ICU::uc
    KF6::KIOCore
    KF6::KIOWidgets
    KF6::Kirigami
    KF6::Prison
    Qt::Concurrent
    Qt::Core
    Qt::Location
    Qt::Multimedia
    Qt::Network
    Qt::Positioning
    Qt::Qml
    Qt::Quick
    Qt::QuickControls2
    Qt::Sql
    Qt::Svg
    Qt::Xml
    QXmpp::QXmpp QXmpp::Omemo
    ${__Qt6Widgets_LIBRARIES}
    ${__KF6Notifications_LIBRARIES}
)

if (TARGET KF6::Crash)
    target_link_libraries(${PROJECT_NAME} KF6::Crash)
endif()

if(ANDROID)
    target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto SQLite::SQLite3 Qt::AndroidExtras)
endif()

if(${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
    target_link_libraries (${PROJECT_NAME} "pthread")
endif()

# Set a custom plist file for the app bundle
if(APPLE)
    if(IOS)
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/ios/Info.plist)
    else()
        set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_SOURCE_DIR}/misc/macos/Info.plist)
    endif()
endif()

# iOS-specific linker flags
if(IOS)
    set(CMAKE_EXE_LINKER_FLAGS ${CMAKE_EXE_LINKER_FLAGS} "-Wl,-e,_qt_main_wrapper -u _qt_registerPlatformPlugin")
endif()

#
# Global C++ definitions
#

if(UBUNTU_TOUCH)
    target_compile_definitions(${PROJECT_NAME} PRIVATE UBUNTU_TOUCH=True)
elseif(APPIMAGE)
    target_compile_definitions(${PROJECT_NAME} PRIVATE
        APPIMAGE=True
        TARGET_GSTREAMER_PLUGINS="${TARGET_GSTREAMER_PLUGINS}"
    )
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
    QXMPP_DISABLE_DEPRECATED_BEFORE=0x010900
    QT_DISABLE_DEPRECATED_UP_TO=0x050F00
    DEBUG_SOURCE_PATH="${CMAKE_SOURCE_DIR}"
)

# Include build directory to use GlobalsGen.h
target_include_directories(${PROJECT_NAME} PUBLIC "${CMAKE_BINARY_DIR}")
