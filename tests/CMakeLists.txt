# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

include(ECMAddTests)

find_package(Qt6Test REQUIRED)

# Include build directory to use GlobalsGen.h
include_directories("${CMAKE_BINARY_DIR}")

ecm_add_test(
	PresenceCacheTest.cpp
	TEST_NAME PresenceCacheTest
	LINK_LIBRARIES Qt::Test libkaidancore
)

ecm_add_test(
	RosterItemWatcherTest.cpp
	TEST_NAME RosterItemWatcherTest
	LINK_LIBRARIES Qt::Test libkaidancore
)
target_compile_definitions(RosterItemWatcherTest PUBLIC UNIT_TEST)

ecm_add_test(
        FutureUtilsTest.cpp
        TEST_NAME FutureUtilsTest
	LINK_LIBRARIES Qt::Test QXmpp::QXmpp
)

ecm_add_test(
	DatabaseTest.cpp
	TEST_NAME DatabaseTest
	LINK_LIBRARIES Qt::Test libkaidancore
)
target_compile_definitions(DatabaseTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	TrustDbTest.cpp
	utils.h
	TEST_NAME TrustDbTest
	LINK_LIBRARIES Qt::Test libkaidancore
)
target_compile_definitions(TrustDbTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	OmemoDbTest.cpp
	utils.h
	TEST_NAME OmemoDbTest
	LINK_LIBRARIES Qt::Test libkaidancore
)
target_compile_definitions(OmemoDbTest PUBLIC DB_UNIT_TEST)

ecm_add_test(
	PublicGroupChatTest.cpp
	TEST_NAME PublicGroupChatTest
	LINK_LIBRARIES Qt::Test libkaidancore
)
ecm_add_test(
	FileModelTest.cpp
	TEST_NAME FileModelTest
	LINK_LIBRARIES Qt::Test libkaidancore
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
