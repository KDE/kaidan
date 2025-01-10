// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QUrl>

#include <QXmppMessage.h>

#ifndef BUILD_TESTS
#include "ClientWorker.h"
#endif

class QGeoCoordinate;

const auto MESSAGE_BUBBLE_PADDING_CHARACTER = u'⠀';
constexpr auto GROUP_CHAT_USER_MENTION_PREFIX = u'@';
constexpr auto GROUP_CHAT_USER_MENTION_SEPARATOR = u' ';

/**
 * This class contains C++ utilities to be used in QML.
 */
class QmlUtils : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QChar messageBubblePaddingCharacter READ messageBubblePaddingCharacter CONSTANT)
    Q_PROPERTY(QChar groupChatUserMentionPrefix READ groupChatUserMentionPrefix CONSTANT)
    Q_PROPERTY(QChar groupChatUserMentionSeparator READ groupChatUserMentionSeparator CONSTANT)
    Q_PROPERTY(QString versionString READ versionString CONSTANT)
    Q_PROPERTY(QString applicationDisplayName READ applicationDisplayName CONSTANT)
    Q_PROPERTY(QUrl applicationWebsiteUrl READ applicationWebsiteUrl CONSTANT)
    Q_PROPERTY(QUrl applicationSourceCodeUrl READ applicationSourceCodeUrl CONSTANT)
    Q_PROPERTY(QUrl issueTrackingUrl READ issueTrackingUrl CONSTANT)
    Q_PROPERTY(QUrl donationUrl READ donationUrl CONSTANT)
    Q_PROPERTY(QUrl mastodonUrl READ mastodonUrl CONSTANT)

public:
    static QmlUtils *instance();

    explicit QmlUtils(QObject *parent = nullptr);
    ~QmlUtils();

    static QChar messageBubblePaddingCharacter();
    static QChar groupChatUserMentionPrefix();
    static QChar groupChatUserMentionSeparator();

    Q_INVOKABLE static QString systemCountryCode();

#ifndef BUILD_TESTS
    /**
     * Returns an error message for a connection error.
     *
     * @param error error for which an error message should be returned
     */
    Q_INVOKABLE static QString connectionErrorMessage(ClientWorker::ConnectionError error);
#endif

    /**
     * Returns a URL to a given resource file name
     *
     * This will check various paths which could contain the searched file.
     * If the file was found, it'll return a `file://` or a `qrc:/` url to
     * the file.
     */
    Q_INVOKABLE static QString getResourcePath(const QString &resourceName);

    /**
     * Returns the version of the current build.
     */
    static QString versionString();

    /**
     * Returns the name of this application as it should be displayed to users.
     */
    static QString applicationDisplayName();

    /**
     * Returns the URL of this application's website.
     */
    Q_INVOKABLE static QUrl applicationWebsiteUrl();

    /**
     * Returns the URL where the source code of this application can be found.
     */
    static QUrl applicationSourceCodeUrl();

    /**
     * Returns the URL to view and report issues.
     */
    static QUrl issueTrackingUrl();

    /**
     * Returns the URL for donations.
     */
    static QUrl donationUrl();

    /**
     * Returns the URL of this application's Mastodon page.
     */
    static QUrl mastodonUrl();

    /**
     * Returns the URL of an XMPP provider's details page.
     *
     * @param providerJid JID of the XMPP provider
     */
    Q_INVOKABLE static QUrl providerDetailsUrl(const QString &providerJid);

    /**
     * Returns an invitation URL for the given XMPP URI.
     */
    Q_INVOKABLE static QUrl invitationUrl(const QString &uri);

#ifndef BUILD_TESTS
    /**
     * Returns an XMPP URI for opening a group chat.
     *
     * @param groupChatJid JID of the group chat
     */
    Q_INVOKABLE static QUrl groupChatUri(const QString &groupChatJid);
#endif

    /**
     * Validates the ID of an encryption key.
     *
     * @param keyId encryption key ID
     *
     * @return whether the key ID is valid
     */
    Q_INVOKABLE static bool validateEncryptionKeyId(const QString &keyId);

    /**
     * Creates a variant of an encryption key ID to be displayed making it easier to read.
     *
     * @param keyId encryption key ID
     *
     * @return displayable key ID
     */
    Q_INVOKABLE static QString displayableEncryptionKeyId(QString keyId);

    /**
     * Returns a string without new lines, unneeded spaces, etc..
     *
     * See QString::simplified for more information.
     */
    Q_INVOKABLE static QString removeNewLinesFromString(const QString &input);

    Q_INVOKABLE static QString quote(QString text);

    /**
     * Copies a URL to the clipboard.
     */
    Q_INVOKABLE static void copyToClipboard(const QUrl &url);

    /**
     * Copies a text to the clipboard.
     */
    Q_INVOKABLE static void copyToClipboard(const QString &text);

    /**
     * Copies an image to the clipboard.
     */
    Q_INVOKABLE static void copyToClipboard(const QImage &image);

    /**
     * Returns a pretty formatted localized data size string.
     */
    Q_INVOKABLE static QString formattedDataSize(qint64 fileSize);

    /**
     * Returns a consistent user color generated from the nickname.
     */
    Q_INVOKABLE static QColor userColor(const QString &id, const QString &name);

    /**
     * Returns a human-readable string describing the state of the chat.
     */
    Q_INVOKABLE static QString chatStateDescription(const QString &displayName, const QXmppMessage::State state);

    Q_INVOKABLE static QString osmUserAgent();

    Q_INVOKABLE static QString geoUri(const QGeoCoordinate &geoCoordinate);
    Q_INVOKABLE static QGeoCoordinate geoCoordinate(const QString &geoUri);

    /**
     * Opens a geo location as preferred by the user.
     *
     * @return whether the location should be opened within Kaidan
     */
    Q_INVOKABLE static bool openGeoLocation(const QGeoCoordinate &geoCoordinate);
};
