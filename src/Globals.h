// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#ifndef GLOBALS_H
#define GLOBALS_H

#include <QLatin1String>

// Kaidan settings
#define KAIDAN_SETTINGS_AUTH_ONLINE "auth/online"
#define KAIDAN_SETTINGS_AUTH_JID "auth/jid"
#define KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX "auth/jidResourcePrefix"
#define KAIDAN_SETTINGS_AUTH_PASSWD "auth/password"
#define KAIDAN_SETTINGS_AUTH_HOST "auth/host"
#define KAIDAN_SETTINGS_AUTH_PORT "auth/port"
#define KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY "auth/passwordVisibility"
#define KAIDAN_SETTINGS_ENCRYPTION "encryption"
#define KAIDAN_SETTINGS_NOTIFICATIONS_MUTED "muted/"
#define KAIDAN_SETTINGS_FAVORITE_EMOJIS "emojis/favorites"
#define KAIDAN_SETTINGS_WINDOW_POSITION "window/position"
#define KAIDAN_SETTINGS_WINDOW_SIZE "window/size"
#define KAIDAN_SETTINGS_AUTOMATIC_MEDIA_DOWNLOADS_RULE "media/automaticDownloadsRule"
#define KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE "explanationVisibility/contactAdditionQrCodePage"
#define KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE "explanationVisibility/keyAuthenticationPage"

#define KAIDAN_JID_RESOURCE_DEFAULT_PREFIX APPLICATION_DISPLAY_NAME

#define INVITATION_URL "https://i.kaidan.im/#"
#define APPLICATION_WEBSITE_URL "https://www.kaidan.im"
#define APPLICATION_SOURCE_CODE_URL "https://invent.kde.org/network/kaidan"
#define ISSUE_TRACKING_URL "https://invent.kde.org/network/kaidan/issues"
#define DONATION_URL "https://liberapay.com/kaidan"
#define MASTODON_URL "https://fosstodon.org/@kaidan"

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
#define GENERATED_PASSWORD_ALPHABET QLatin1String( \
	"abcdefghijklmnopqrstuvwxyz" \
	"ABCDEFGHIJKLMNOPQRSTUVWXYZ" \
	"`1234567890-=" \
	"~!@#$%^&*()_+" \
	"[];'\\,./{}:\"|<>?" \
)

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

constexpr QStringView DEFAULT_LANGUAGE_CODE = u"EN";
constexpr QStringView DEFAULT_COUNTRY_CODE = u"US";

/**
 * Number of providers required in a country so that only providers from that country are
 * randomly selected.
 */
constexpr auto PROVIDER_LIST_MIN_PROVIDERS_FROM_COUNTRY = 2;

/**
 * Name of the @c QQuickImageProvider for Bits of Binary.
 */
#define BITS_OF_BINARY_IMAGE_PROVIDER_NAME "bits-of-binary"

// JPEG export quality used when saving images lossy (e.g. when saving images from clipboard)
constexpr auto JPEG_EXPORT_QUALITY = 85;
// Maximum file size for reading files just to generate an image thumbnail.
constexpr auto THUMBNAIL_GENERATION_MAX_FILE_SIZE = 10 * 1024 * 1024;
// Width and height of generated file thumbnails.
constexpr auto THUMBNAIL_PIXEL_SIZE = 50;

// Count of encryption key ID characters that are grouped to be displayed for better readability
constexpr int ENCRYPTION_KEY_ID_CHARACTER_GROUP_SIZE = 8;

// Separator between grouped encryption key ID characters to be displayed for better readability
constexpr QStringView ENCRYPTION_KEY_ID_CHARACTER_GROUP_SEPARATOR = u" ";

#endif // GLOBALS_H
