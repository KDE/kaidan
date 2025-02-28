# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

include(ECMAddTests)

find_package(Qt6Test REQUIRED)

# Include build directory to use GlobalsGen.h
include_directories(PUBLIC "${CMAKE_BINARY_DIR}")

ecm_add_test(
	PresenceCacheTest.cpp
	../src/PresenceCache.cpp
	../src/PresenceCache.h
	TEST_NAME PresenceCacheTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)

ecm_add_test(
	RosterItemWatcherTest.cpp
	../src/RosterItemWatcher.cpp
	../src/RosterItemWatcher.h
	../src/RosterItem.cpp
	../src/RosterItem.h
	TEST_NAME RosterItemWatcherTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)
target_compile_definitions(RosterItemWatcherTest PUBLIC UNIT_TEST)

ecm_add_test(
	UserPresenceWatcherTest.cpp
	../src/PresenceCache.cpp
	../src/PresenceCache.h
	TEST_NAME UserPresenceWatcherTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)

ecm_add_test(
        FutureUtilsTest.cpp
        TEST_NAME FutureUtilsTest
        LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)

ecm_add_test(
	DatabaseTest.cpp
	../src/Database.cpp
	../src/SqlUtils.cpp
	TEST_NAME DatabaseTest
	LINK_LIBRARIES Qt::Test Qt::Sql QXmpp::QXmpp
)
target_compile_definitions(DatabaseTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	TrustDbTest.cpp
	utils.h
	../src/Database.cpp
	../src/Database.h
	../src/DatabaseComponent.cpp
	../src/DatabaseComponent.h
	../src/SqlUtils.cpp
	../src/SqlUtils.h
	../src/TrustDb.cpp
	../src/TrustDb.h
	TEST_NAME TrustDbTest
	LINK_LIBRARIES Qt::Test Qt::Gui Qt::Sql QXmpp::QXmpp
)
target_compile_definitions(TrustDbTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	OmemoDbTest.cpp
	utils.h
	../src/Database.cpp
	../src/Database.h
	../src/DatabaseComponent.cpp
	../src/DatabaseComponent.h
	../src/SqlUtils.cpp
	../src/SqlUtils.h
	../src/OmemoDb.cpp
	../src/OmemoDb.h
	TEST_NAME OmemoDbTest
	LINK_LIBRARIES Qt::Test Qt::Gui Qt::Sql QXmpp::QXmpp QXmpp::Omemo
)
target_compile_definitions(OmemoDbTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	PublicGroupChatTest.cpp
	../src/PublicGroupChat.cpp
	../src/PublicGroupChat.h
	../src/PublicGroupChatModel.cpp
	../src/PublicGroupChatModel.h
	../src/PublicGroupChatProxyModel.cpp
	../src/PublicGroupChatProxyModel.h
	../src/PublicGroupChatSearchManager.cpp
	../src/PublicGroupChatSearchManager.h
	TEST_NAME PublicGroupChatTest
	LINK_LIBRARIES Qt::Test Qt::Network
)

ecm_add_test(
	FileModelTest.cpp
	../src/Enums.h
	../src/MediaUtils.cpp
	../src/MediaUtils.h
	../src/Message.cpp
	../src/Message.h
	../src/FileModel.cpp
	../src/FileModel.h
	../src/FileProxyModel.cpp
	../src/FileProxyModel.h
    ../src/QmlUtils.cpp
    ../src/QmlUtils.h
	TEST_NAME FileModelTest
	LINK_LIBRARIES Qt::Gui Qt::Positioning Qt::Concurrent Qt::Sql Qt::Test QXmpp::QXmpp KF6::KIOFileWidgets
)

target_compile_definitions(FileModelTest PRIVATE BUILD_TESTS)

# Manual tests

add_executable(PublicGroupChatSearch
	manual/PublicGroupChatSearch.cpp
)
target_link_libraries(PublicGroupChatSearch
	PRIVATE
		Qt::Core Qt::Network
)

add_executable(SendFileSourcesAttachment
	manual/SendFileSourcesAttachment.cpp
)
target_link_libraries(SendFileSourcesAttachment
	PRIVATE Qt::Core QXmpp::QXmpp
)
