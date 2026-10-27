// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
#include <variant>
// Qt
#include <QList>
#include <QString>
// QXmpp
#include <QXmppConfiguration.h>
#include <QXmppError.h>
// Kaidan
#include "Account.h"
#include "Encryption.h"
#include "RosterItem.h"

class QDomElement;
class QXmlStreamWriter;

inline constexpr QStringView s_client_settings = u"client-settings";
inline constexpr QStringView s_old_configuration = u"old-configuration";
inline constexpr QStringView s_roster = u"roster";
inline constexpr QStringView s_item = u"item";
inline constexpr QStringView s_kaidan_export_ns = u"im.kaidan.export";

struct ClientRosterItemSettings {
    ClientRosterItemSettings() = default;
    explicit ClientRosterItemSettings(const RosterItem &item);

    static std::variant<ClientRosterItemSettings, QXmppError> fromDom(const QDomElement &rootElement);
    void toXml(QXmlStreamWriter &writer) const;

    QString jid;
    std::optional<Encryption::Enum> encryption;
    std::optional<RosterItem::NotificationRule> notificationRule;
    std::optional<bool> chatStateSendingEnabled;
    std::optional<bool> readMarkerSendingEnabled;
    std::optional<int> pinningPosition;
    std::optional<RosterItem::AutomaticMediaDownloadsRule> automaticMediaDownloadsRule;
};

struct ClientConfiguration {
    ClientConfiguration() = default;
    explicit ClientConfiguration(const AccountSettings *accountSettings);

    static std::variant<ClientConfiguration, QXmppError> fromDom(const QDomElement &rootElement);
    void toXml(QXmlStreamWriter &writer) const;

    QXmppConfiguration toXmppConfiguration() const;

    std::optional<QString> jid;
    std::optional<QString> password;
    std::optional<QString> host;
    std::optional<quint16> port;
    std::optional<bool> tlsErrorsIgnored;
    std::optional<QXmppConfiguration::StreamSecurityMode> tlsRequirement;
    std::optional<AccountSettings::PasswordVisibility> passwordVisibility;
    std::optional<Encryption::Enum> encryption;
    std::optional<AccountSettings::AutomaticMediaDownloadsRule> automaticMediaDownloadsRule;
    std::optional<QString> name;
    std::optional<AccountSettings::ContactNotificationRule> contactNotificationRule;
    std::optional<AccountSettings::GroupChatNotificationRule> groupChatNotificationRule;
    std::optional<bool> geoLocationMapPreviewEnabled;
    std::optional<AccountSettings::GeoLocationMapService> geoLocationMapService;
};

struct ClientSettings {
    ClientSettings() = default;
    explicit ClientSettings(AccountSettings *accountSettings, const QList<RosterItem> &rosterItems);

    QList<QString> rosterContacts() const;

    static std::variant<ClientSettings, QXmppError> fromDom(const QDomElement &rootElement);
    void toXml(QXmlStreamWriter &writer) const;

    ClientConfiguration oldConfiguration;
    QList<ClientRosterItemSettings> roster;
};
