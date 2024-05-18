# SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

#
# Sources / Resources
#

add_executable(${PROJECT_NAME} WIN32)

target_sources(${PROJECT_NAME} PRIVATE
	AbstractNotifier.h
	Account.cpp
	Account.h
	AccountDb.cpp
	AccountDb.h
	AccountManager.cpp
	AccountManager.h
	Algorithms.h
	AtmManager.cpp
	AtmManager.h
	AudioDeviceModel.cpp
	AudioDeviceModel.h
	AvatarFileStorage.cpp
	AvatarFileStorage.h
	BitsOfBinaryImageProvider.cpp
	BitsOfBinaryImageProvider.h
	Blocking.cpp
	Blocking.h
	CameraImageCapture.cpp
	CameraImageCapture.h
	CameraModel.cpp
	CameraModel.h
	ChatHintModel.cpp
	ChatHintModel.h
	ClientWorker.cpp
	ClientWorker.h
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
	LogHandler.cpp
	LogHandler.h
	main.cpp
	MediaRecorder.cpp
	MediaRecorder.h
	MediaSettingModel.h
	MediaSettings.cpp
	MediaSettings.h
	MediaUtils.cpp
	MediaUtils.h
	MessageComposition.cpp
	MessageComposition.h
	Message.cpp
	MessageDb.cpp
	MessageDb.h
	Message.h
	MessageHandler.cpp
	MessageHandler.h
	MessageModel.cpp
	MessageModel.h
	Notifications.cpp
	Notifications.h
	OmemoCache.cpp
	OmemoCache.h
	OmemoDb.cpp
	OmemoDb.h
	OmemoManager.cpp
	OmemoManager.h
	OmemoModel.cpp
	OmemoModel.h
	OmemoWatcher.cpp
	OmemoWatcher.h
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
	QrCodeDecoder.cpp
	QrCodeDecoder.h
	QrCodeGenerator.cpp
	QrCodeGenerator.h
	QrCodeScannerFilter.cpp
	QrCodeScannerFilter.h
	QrCodeVideoFrame.cpp
	QrCodeVideoFrame.h
	# kaidan QXmpp extensions (need to be merged into QXmpp upstream)
	qxmpp-exts/QXmppColorGenerator.cpp
	qxmpp-exts/QXmppColorGenerator.h
	qxmpp-exts/QXmppUri.cpp
	qxmpp-exts/QXmppUri.h
	RecentPicturesModel.cpp
	RecentPicturesModel.h
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
	TrustDb.cpp
	TrustDb.h
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

# QML files
if(QUICK_COMPILER)
	qtquick_compiler_add_resources(COMPILED_QML_QRC qml/qml.qrc)
	target_sources(${PROJECT_NAME} PRIVATE ${COMPILED_QML_QRC})
else()
	target_sources(${PROJECT_NAME} PRIVATE qml/qml.qrc)
endif()

ecm_create_qm_loader(${PROJECT_NAME} kaidan_qt)

target_link_libraries(${PROJECT_NAME}
	Qt::Core
	Qt::Sql
	Qt::Qml
	Qt::Quick
	Qt::Svg
	Qt::Network
	Qt::Xml
	Qt::Multimedia
	Qt::Positioning
	Qt::Location
	Qt::QuickControls2
	KF5::CoreAddons
	KF5::KIOCore
	KF5::KIOWidgets
	QXmpp::QXmpp QXmpp::Omemo
	${__Qt5Widgets_LIBRARIES}
	${__KF5Notifications_LIBRARIES}
)

if(TARGET ZXing::ZXing)
        target_link_libraries(${PROJECT_NAME} ZXing::ZXing)
elseif(TARGET ZXing::Core)
        target_link_libraries(${PROJECT_NAME} ZXing::Core)
endif()

if(ANDROID OR IOS)
	target_link_libraries(${PROJECT_NAME} Qt5::QuickControls2 KF5::Kirigami2)
endif()

if(ANDROID)
	target_link_libraries(${PROJECT_NAME} OpenSSL::Crypto SQLite::SQLite3 Qt5::AndroidExtras)
else()
	target_link_libraries(${PROJECT_NAME} KF5::KIOFileWidgets)
endif()

if(STATIC_BUILD)
	add_definitions(-DQXMPP_BUILD)

	find_package(Perl REQUIRED)

	set(STATIC_DEPENDENCIES_CMAKE_FILE "${CMAKE_BINARY_DIR}/QtStaticDependencies.cmake")
	if(EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
		file(REMOVE ${STATIC_DEPENDENCIES_CMAKE_FILE})
	endif()

	get_target_property(QT_LIBDIR Qt5::Core LOCATION)
	get_filename_component(QT_LIBDIR ${QT_LIBDIR} DIRECTORY)

	macro(CONVERT_PRL_LIBS_TO_CMAKE _qt_component)
		if(TARGET Qt5::${_qt_component})
			get_target_property(_lib_location Qt5::${_qt_component} LOCATION)
			execute_process(COMMAND ${PERL_EXECUTABLE} ${CMAKE_SOURCE_DIR}/utils/convert-prl-libs-to-cmake.pl
				--lib ${_lib_location}
				--libdir ${QT_LIBDIR}
				--out ${STATIC_DEPENDENCIES_CMAKE_FILE}
				--component ${_qt_component}
				--compiler ${CMAKE_CXX_COMPILER_ID}
			)
		endif()
	endmacro()

	link_directories(${_qt5_install_prefix}/../)

	foreach(qt_module Gui Quick QuickControls2 Network Qml Svg Sql QSQLiteDriverPlugin
             QJpegPlugin QGifPlugin QSvgPlugin QSvgIconPlugin QICOPlugin QGenericEnginePlugin
	         QLocalClientConnectionFactory QTcpServerConnectionFactory)
		CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
	endforeach()

	if(IOS)
		foreach(qt_module QIOSIntegrationPlugin QMacHeifPlugin QMacJp2Plugin
			QICNSPlugin QTgaPlugin QTiffPlugin QWbmpPlugin QWebpPlugin)
			CONVERT_PRL_LIBS_TO_CMAKE(${qt_module})
		endforeach()
	endif()

	if(NOT EXISTS ${STATIC_DEPENDENCIES_CMAKE_FILE})
		message(FATAL_ERROR "Unable to find ${STATIC_DEPENDENCIES_CMAKE_FILE}")
	endif()
	include(${STATIC_DEPENDENCIES_CMAKE_FILE})

	set(QT_QML_PATH ${_qt5Quick_install_prefix})

	find_library(KIRIGAMI_PLUGIN kirigamiplugin PATHS ${Kirigami2_INSTALL_PREFIX}/${KDE_INSTALL_QMLDIR}/org/kde/kirigami.2)
	find_library(QUICK_PLUGIN qtquick2plugin PATHS ${QT_QML_PATH}/qml/QtQuick.2)
	find_library(LABS_PLATFORM_PLUGIN qtlabsplatformplugin PATHS ${QT_QML_PATH}/qml/Qt/labs/platform)
	find_library(GRAPHEFFECTS_PLUGIN qtgraphicaleffectsplugin PATHS ${QT_QML_PATH}/qml/QtGraphicalEffects)
	find_library(GRAPHEFFECTS_PRIVATE_PLUGIN qtgraphicaleffectsprivate PATHS ${QT_QML_PATH}/qml/QtGraphicalEffects/private)
	find_library(QQC2_PLUGIN qtquickcontrols2plugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2)
	find_library(QQC2_MATERIAL_PLUGIN qtquickcontrols2materialstyleplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2/Material)
	find_library(QQC2_UNIVERSAL_PLUGIN qtquickcontrols2universalstyleplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Controls.2/Universal)
	find_library(QLAYOUTS_PLUGIN qquicklayoutsplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Layouts)
	find_library(QDIALOGS_PLUGIN qquickdialogsplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Dialogs)
	find_library(QWINDOW_PLUGIN windowplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Window.2)
	find_library(QSHAPES_PLUGIN qmlshapesplugin PATHS ${QT_QML_PATH}/qml/QtQuick/Shapes)
	find_library(QUICKSHAPES Qt5QuickShapes PATHS ${QT_LIBDIR})
	find_library(QTEMPLATES_PLUGIN qtquicktemplates2plugin PATHS ${QT_QML_PATH}/qml/QtQuick/Templates.2)
	find_library(QMODELS_PLUGIN modelsplugin PATHS ${QT_QML_PATH}/qml/QtQml/Models.2)
	target_link_libraries(${PROJECT_NAME}
		${plugin_libs}
		Qt5::QSQLiteDriverPlugin
		Qt5::QJpegPlugin
		Qt5::QGifPlugin
		Qt5::QSvgPlugin
		Qt5::QSvgIconPlugin
		Qt5::QICOPlugin
		Qt5::QGenericEnginePlugin
		Qt5::QLocalClientConnectionFactory
		Qt5::QTcpServerConnectionFactory
		${KIRIGAMI_PLUGIN}
		${QUICK_PLUGIN}
		${LABS_PLATFORM_PLUGIN}
		${GRAPHEFFECTS_PLUGIN}
		${GRAPHEFFECTS_PRIVATE_PLUGIN}
		${QQC2_PLUGIN}
		${QQC2_MATERIAL_PLUGIN}
		${QQC2_UNIVERSAL_PLUGIN}
		${QLAYOUTS_PLUGIN}
		${QDIALOGS_PLUGIN}
		${QWINDOW_PLUGIN}
		${QSHAPES_PLUGIN}
		${QUICKSHAPES}
		${QTEMPLATES_PLUGIN}
		${QMODELS_PLUGIN}
		${__Qt5Widgets_LIBRARIES}
	)

	if(IOS)
		target_link_libraries(${PROJECT_NAME}
			Qt5::QIOSIntegrationPlugin
			Qt5::QMacHeifPlugin
			Qt5::QMacJp2Plugin
			Qt5::QICNSPlugin
			Qt5::QTgaPlugin
			Qt5::QTiffPlugin
			Qt5::QWbmpPlugin
			Qt5::QWebpPlugin
		)
	endif()
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

if(STATIC_BUILD)
	target_compile_definitions(${PROJECT_NAME} PRIVATE
		STATIC_BUILD=True
	)
endif()

target_compile_definitions(${PROJECT_NAME} PRIVATE
	QXMPP_DISABLE_DEPRECATED_BEFORE=0x010400
	QT_DISABLE_DEPRECATED_BEFORE=0x050F00
	DEBUG_SOURCE_PATH="${CMAKE_SOURCE_DIR}"
	VERSION_STRING="${VERSION_STRING}"
	APPLICATION_ID="${APPLICATION_ID}"
	APPLICATION_NAME="${APPLICATION_NAME}"
	APPLICATION_DISPLAY_NAME="${APPLICATION_DISPLAY_NAME}"
	APPLICATION_DESCRIPTION="${APPLICATION_DESCRIPTION}"
)
