# SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
#
# SPDX-License-Identifier: CC0-1.0

include(ECMAddTests)

find_package(Qt6Test REQUIRED)

# Include build directory to use GlobalsGen.h
include_directories("${CMAKE_BINARY_DIR}")

# Intermediate library shared by all tests
add_library(libkaidantests STATIC)
add_library(Kaidan::Tests ALIAS libkaidantests)

target_sources(libkaidantests
    PUBLIC
    Test.cpp
    Test.h
    TestUtils.h
)

target_link_libraries(libkaidantests
    PUBLIC
    libkaidancore
    Qt::Test
)


ecm_add_test(
    DatabaseTest.cpp
    TEST_NAME DatabaseTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    FileModelTest.cpp
    TEST_NAME FileModelTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    FutureUtilsTest.cpp
    TEST_NAME FutureUtilsTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    OmemoDbTest.cpp
    TEST_NAME OmemoDbTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    PresenceCacheTest.cpp
    TEST_NAME PresenceCacheTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    PublicGroupChatTest.cpp
    TEST_NAME PublicGroupChatTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    RosterItemWatcherTest.cpp
    TEST_NAME RosterItemWatcherTest
    LINK_LIBRARIES Kaidan::Tests
)

ecm_add_test(
    TrustDbTest.cpp
    TEST_NAME TrustDbTest
    LINK_LIBRARIES Kaidan::Tests
)

# Manual tests

add_executable(PublicGroupChatSearch
    manual/PublicGroupChatSearch.cpp
)
target_link_libraries(PublicGroupChatSearch
    PRIVATE Qt::Core Qt::Network
)

add_executable(SendFileSourcesAttachment
    manual/SendFileSourcesAttachment.cpp
)
target_link_libraries(SendFileSourcesAttachment
    PRIVATE Qt::Core QXmpp::QXmpp
)
