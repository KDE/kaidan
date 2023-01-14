# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

include(ECMAddTests)

find_package(Qt5Test REQUIRED)

ecm_add_test(
	PresenceCacheTest.cpp
	../src/PresenceCache.cpp
	TEST_NAME PresenceCacheTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)

ecm_add_test(
	RosterItemWatcherTest.cpp
	../src/RosterItemWatcher.cpp
	../src/RosterItem.cpp
	TEST_NAME RosterItemWatcherTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)
target_compile_definitions(RosterItemWatcherTest PUBLIC UNIT_TEST)

ecm_add_test(
	UserPresenceWatcherTest.cpp
	../src/PresenceCache.cpp
	TEST_NAME UserPresenceWatcherTest
	LINK_LIBRARIES Qt::Test Qt::Gui QXmpp::QXmpp
)

ecm_add_test(
	TrustDbTest.cpp
	../src/Database.cpp
	../src/DatabaseComponent.cpp
	../src/SqlUtils.cpp
	../src/TrustDb.cpp
	TEST_NAME TrustDbTest
	LINK_LIBRARIES Qt::Test Qt::Gui Qt::Sql QXmpp::QXmpp
)
target_compile_definitions(TrustDbTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	OmemoDbTest.cpp
	../src/Database.cpp
	../src/DatabaseComponent.cpp
	../src/SqlUtils.cpp
	../src/OmemoDb.cpp
	TEST_NAME OmemoDbTest
	LINK_LIBRARIES Qt::Test Qt::Gui Qt::Sql QXmpp::QXmpp QXmpp::Omemo
)
target_compile_definitions(OmemoDbTest PUBLIC DB_UNIT_TEST)

# Manual tests

add_executable(PublicGroupChatSearch
	manual/public-group-chat-search.cpp
)
target_link_libraries(PublicGroupChatSearch
	PRIVATE
		Qt::Core Qt::Network
)
