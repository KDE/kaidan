// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QLatin1String>
#include <QObject>
// Kaidan
#include <GlobalsGen.h>

const auto PROFILE_VARIABLE = "KAIDAN_PROFILE";

// Kaidan settings
#define KAIDAN_SETTINGS_FAVORITE_EMOJIS "emojis/favorites"
#define KAIDAN_SETTINGS_WINDOW_POSITION "window/position"
#define KAIDAN_SETTINGS_WINDOW_SIZE "window/size"
#define KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE "explanationVisibility/contactAdditionQrCodePage"
#define KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE "explanationVisibility/keyAuthenticationPage"

#define INVITATION_URL "https://i.kaidan.im/#"
#define APPLICATION_WEBSITE_URL "https://www.kaidan.im"
#define APPLICATION_SOURCE_CODE_URL "https://invent.kde.org/network/kaidan"
#define ISSUE_TRACKING_URL "https://invent.kde.org/network/kaidan/issues"
#define DONATION_URL "https://liberapay.com/kaidan"
#define MASTODON_URL "https://fosstodon.org/@kaidan"
inline constexpr QStringView PROVIDER_DETAILS_URL = u"https://providers.xmpp.net/provider/%1/";

constexpr auto MESSAGE_MAX_CHARS = 1e4;

// SQL
#define DB_FILE_BASE_NAME "kaidan"
#define DB_TABLE_INFO "dbinfo"
#define DB_TABLE_ACCOUNTS "accounts"
#define DB_TABLE_ROSTER "roster"
#define DB_TABLE_ROSTER_GROUPS "rosterGroups"
#define DB_TABLE_MESSAGES "messages"
#define DB_VIEW_CHAT_MESSAGES "chatMessages"
#define DB_VIEW_DRAFT_MESSAGES "draftMessages"
#define DB_TABLE_GROUP_CHAT_USERS "groupChatUsers"
#define DB_TABLE_FILES "files"
#define DB_TABLE_FILE_HASHES "fileHashes"
#define DB_TABLE_FILE_HTTP_SOURCES "fileHttpSources"
#define DB_TABLE_FILE_ENCRYPTED_SOURCES "fileEncryptedSources"
#define DB_TABLE_MESSAGE_REACTIONS "messageReactions"
#define DB_TABLE_BLOCKED "blocked"
#define DB_TABLE_TRUST_SECURITY_POLICIES "trustSecurityPolicies"
#define DB_TABLE_TRUST_OWN_KEYS "trustOwnKeys"
#define DB_TABLE_TRUST_KEYS "trustKeys"
#define DB_TABLE_TRUST_KEYS_UNPROCESSED "trustKeysUnprocessed"
#define DB_TABLE_OMEMO_DEVICES "omemoDevices"
#define DB_TABLE_OMEMO_OWN_DEVICES "omemoDevicesOwn"
#define DB_TABLE_OMEMO_PRE_KEY_PAIRS "omemoPreKeyPairs"
#define DB_TABLE_OMEMO_SIGNED_PRE_KEY_PAIRS "omemoPreKeyPairsSigned"
#define DB_TABLE_ROSTER_GROUPS "rosterGroups"
#define DB_QUERY_LIMIT_MESSAGES 20
#define DB_QUERY_LIMIT_GROUP_CHAT_USERS 80

//
// Credential generation
//

// Length of generated usernames
constexpr auto GENERATED_USERNAME_LENGTH = 6;

// Lower bound of the length for generated passwords (inclusive)
constexpr auto GENERATED_PASSWORD_LENGTH_LOWER_BOUND = 20;

// Upper bound of the length for generated passwords (inclusive)
constexpr auto GENERATED_PASSWORD_LENGTH_UPPER_BOUND = 30;

// Characters used for password generation
#define GENERATED_PASSWORD_ALPHABET                                                                                                                            \
    QLatin1String(                                                                                                                                             \
        "abcdefghijklmnopqrstuvwxyz"                                                                                                                           \
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"                                                                                                                           \
        "`1234567890-="                                                                                                                                        \
        "~!@#$%^&*()_+"                                                                                                                                        \
        "[];'\\,./{}:\"|<>?")

// Number of characters used for password generation
#define GENERATED_PASSWORD_ALPHABET_LENGTH GENERATED_PASSWORD_ALPHABET.size()

/**
 * Path of the JSON provider list file
 */
#define PROVIDER_LIST_FILE_PATH QStringLiteral(":/data/providers.json")

/**
 * Path of the JSON provider completion list file
 */
#define PROVIDER_COMPLETION_LIST_FILE_PATH QStringLiteral(":/data/providers-completion.json")

const auto NO_SELECTION_TEXT = QObject::tr("No selection");
constexpr QStringView DEFAULT_LANGUAGE_CODE = u"EN";
constexpr QStringView DEFAULT_COUNTRY_CODE = u"US";

/**
 * Number of providers required in a country so that only providers from that country are
 * randomly selected.
 */
constexpr auto PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY = 2;

// Name of the provider for all images.
const QString IMAGE_PROVIDER_NAME = QStringLiteral("image-provider");

// Defines that a message is suitable for correction only if it is among the n most recent messages.
constexpr int MAX_MESSAGE_CORRECTION_COUNT = 10;
// Defines that a message is suitable for correction only if it has not been sent earlier than n days ago.
constexpr int MAX_MESSAGE_CORRECTION_DAYS = 2;

// Quality of generated file thumbnails.
constexpr auto THUMBNAIL_QUALITY = 85;
// Width and height of generated file thumbnails.
constexpr auto THUMBNAIL_EDGE_PIXEL_COUNT = 30;
// Width and height of generated video thumbnails.
constexpr auto VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT = 500;

// Count of encryption key ID characters that are grouped to be displayed for better readability
constexpr int ENCRYPTION_KEY_ID_CHARACTER_GROUP_SIZE = 8;

// Separator between grouped encryption key ID characters to be displayed for better readability
constexpr QStringView ENCRYPTION_KEY_ID_CHARACTER_GROUP_SEPARATOR = u" ";

// XMPP
// TODO: Find a solution to not define namespaces in Kaidan
inline constexpr QStringView XMLNS_SFS = u"urn:xmpp:sfs:0";
inline constexpr QStringView XMLNS_MESSAGE_REPLIES = u"urn:xmpp:reply:0";
const auto XMLNS_OMEMO_2 = QStringLiteral("urn:xmpp:omemo:2");
