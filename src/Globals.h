/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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
#define KAIDAN_SETTINGS_NOTIFICATIONS_MUTED "muted/"
#define KAIDAN_SETTINGS_FAVORITE_EMOJIS "emojis/favorites"
#define KAIDAN_SETTINGS_WINDOW_SIZE "window/size"
#define KAIDAN_SETTINGS_HELP_VISIBILITY_QR_CODE_PAGE "helpVisibility/qrCodePage"

#define KAIDAN_JID_RESOURCE_DEFAULT_PREFIX APPLICATION_DISPLAY_NAME

#define INVITATION_URL "https://i.kaidan.im/#"
#define APPLICATION_SOURCE_CODE_URL "https://invent.kde.org/network/kaidan"
#define ISSUE_TRACKING_URL "https://invent.kde.org/network/kaidan/issues"

constexpr auto MESSAGE_MAX_CHARS = 1e4;

// SQL
#define DB_FILE_BASE_NAME "messages"
#define DB_TABLE_INFO "dbinfo"
#define DB_TABLE_ROSTER "Roster"
#define DB_TABLE_MESSAGES "messages"
#define DB_TABLE_FILES "files"
#define DB_TABLE_FILE_HASHES "file_hashes"
#define DB_TABLE_FILE_HTTP_SOURCES "file_http_sources"
#define DB_TABLE_FILE_ENCRYPTED_SOURCES "file_encrypted_sources"
#define DB_TABLE_TRUST_SECURITY_POLICIES "trust_security_policies"
#define DB_TABLE_TRUST_OWN_KEYS "trust_own_keys"
#define DB_TABLE_TRUST_KEYS "trust_keys"
#define DB_TABLE_TRUST_KEYS_UNPROCESSED "trust_keys_unprocessed"
#define DB_TABLE_OMEMO_DEVICES "omemo_devices"
#define DB_TABLE_OMEMO_OWN_DEVICES "omemo_devices_own"
#define DB_TABLE_OMEMO_PRE_KEY_PAIRS "omemo_pre_key_pairs"
#define DB_TABLE_OMEMO_SIGNED_PRE_KEY_PAIRS "omemo_pre_key_pairs_signed"
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

#endif // GLOBALS_H
