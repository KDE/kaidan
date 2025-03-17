// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QDateTime>
#include <QObject>
#include <QString>
#include <QUuid>
// QXmpp
#include <QXmppConfiguration.h>
#include <QXmppCredentials.h>
// Kaidan
#include "Encryption.h"
#include "Globals.h"
#include "Kaidan.h"

constexpr quint16 PORT_AUTODETECT = 0;

class Account
{
    Q_GADGET

    Q_PROPERTY(bool online MEMBER online CONSTANT FINAL)
    Q_PROPERTY(QString jid MEMBER jid CONSTANT FINAL)
    Q_PROPERTY(QString resourcePrefix MEMBER resourcePrefix CONSTANT FINAL)
    Q_PROPERTY(QString password MEMBER password CONSTANT FINAL)
    Q_PROPERTY(QXmppCredentials credentials MEMBER credentials CONSTANT FINAL)
    Q_PROPERTY(QString host MEMBER host CONSTANT FINAL)
    Q_PROPERTY(quint16 port MEMBER port CONSTANT FINAL)
    Q_PROPERTY(bool tlsErrorsIgnored MEMBER tlsErrorsIgnored CONSTANT FINAL)
    Q_PROPERTY(QXmppConfiguration::StreamSecurityMode tlsRequirement MEMBER tlsRequirement CONSTANT FINAL)
    Q_PROPERTY(Kaidan::PasswordVisibility passwordVisibility MEMBER passwordVisibility CONSTANT FINAL)
    Q_PROPERTY(QUuid userAgentDeviceId MEMBER userAgentDeviceId CONSTANT FINAL)
    Q_PROPERTY(Encryption::Enum encryption MEMBER encryption CONSTANT FINAL)
    Q_PROPERTY(Account::AutomaticMediaDownloadsRule automaticMediaDownloadsRule MEMBER automaticMediaDownloadsRule CONSTANT FINAL)
    Q_PROPERTY(QString name MEMBER name CONSTANT FINAL)
    Q_PROPERTY(Account::ContactNotificationRule contactNotificationRule MEMBER contactNotificationRule CONSTANT FINAL)
    Q_PROPERTY(Account::GroupChatNotificationRule groupChatNotificationRule MEMBER groupChatNotificationRule CONSTANT FINAL)
    Q_PROPERTY(bool geoLocationMapPreviewEnabled MEMBER geoLocationMapPreviewEnabled CONSTANT FINAL)
    Q_PROPERTY(Account::GeoLocationMapService geoLocationMapService MEMBER geoLocationMapService CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName)
    Q_PROPERTY(QString jidResource READ jidResource)

public:
    /**
     * Default rule to inform the user about incoming messages from contacts.
     */
    enum class ContactNotificationRule {
        Never, ///< Never notify.
        PresenceOnly, ///< Notify only for contacts receiving presence.
        Always, ///< Always notify.
        Default = Always,
    };
    Q_ENUM(ContactNotificationRule)

    /**
     * Default rule to inform the user about incoming messages from group chats.
     */
    enum class GroupChatNotificationRule {
        Never, ///< Never notify.
        Mentioned, ///< Notify only if the user is mentioned.
        Always, ///< Always notify.
        Default = Always,
    };
    Q_ENUM(GroupChatNotificationRule)

    /**
     * Default rule to automatically download media for all roster items of an account.
     */
    enum class AutomaticMediaDownloadsRule {
        Never, ///< Never automatically download files.
        PresenceOnly, ///< Automatically download files only for contacts receiving presence.
        Always, ///< Always automatically download files.
        Default = PresenceOnly,
    };
    Q_ENUM(AutomaticMediaDownloadsRule)

    /**
     * Map service for opening geo locations.
     */
    enum class GeoLocationMapService {
        System, ///< Let the system decide how to open geo locations.
        InApp, ///< Open geo locations in Kaidan.
        Web, ///< Open geo locations in a web browser.
    };
    Q_ENUM(GeoLocationMapService)

    Account() = default;

    QString displayName() const;
    QString jidResource() const;

    bool isDefaultPort() const;

    bool operator==(const Account &other) const = default;
    bool operator!=(const Account &other) const = default;

    bool operator<(const Account &other) const;
    bool operator>(const Account &other) const;
    bool operator<=(const Account &other) const;
    bool operator>=(const Account &other) const;

    bool online = true;
    QString jid;
    QString resourcePrefix = QStringLiteral(KAIDAN_JID_RESOURCE_DEFAULT_PREFIX);
    QString password;
    QXmppCredentials credentials;
    QString host;
    quint16 port = PORT_AUTODETECT;
    bool tlsErrorsIgnored = false;
    QXmppConfiguration::StreamSecurityMode tlsRequirement = QXmppConfiguration::TLSRequired;
    Kaidan::PasswordVisibility passwordVisibility = Kaidan::PasswordVisible;
    QUuid userAgentDeviceId;
    Encryption::Enum encryption = Encryption::Omemo2;
    Account::AutomaticMediaDownloadsRule automaticMediaDownloadsRule = Account::AutomaticMediaDownloadsRule::Default;
    QString name;
    QString latestMessageStanzaId;
    QDateTime latestMessageStanzaTimestamp;
    qint64 httpUploadLimit = 0;
    ContactNotificationRule contactNotificationRule = ContactNotificationRule::Default;
    GroupChatNotificationRule groupChatNotificationRule = GroupChatNotificationRule::Default;
    bool geoLocationMapPreviewEnabled = true;
    GeoLocationMapService geoLocationMapService = GeoLocationMapService::System;

private:
    // This is dynamic and not saved into db
    mutable QString m_jidResource;
};

Q_DECLARE_METATYPE(Account)
Q_DECLARE_METATYPE(Account::ContactNotificationRule)
Q_DECLARE_METATYPE(Account::GroupChatNotificationRule)
Q_DECLARE_METATYPE(Account::AutomaticMediaDownloadsRule)
Q_DECLARE_METATYPE(QXmppConfiguration::StreamSecurityMode)
