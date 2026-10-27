// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationData.h"

// Qt
#include <QDomElement>
#include <QXmlStreamWriter>
// Kaidan
#include "Algorithms.h"
#include "XmlUtils.h"

#define readDataElement(NAME)                                                                                                                                  \
    if (!XmlUtils::Reader::text(rootElement.firstChildElement(QStringLiteral(#NAME)), data.NAME)) {                                                            \
        return QXmppError{QStringLiteral("Invalid NAME element."), {}};                                                                                        \
    }

#define writeDataElement(NAME) XmlUtils::Writer::text(&writer, QStringLiteral(#NAME), NAME)

ClientRosterItemSettings::ClientRosterItemSettings(const RosterItem &item)
    : jid(item.jid)
    , encryption(item.encryption)
    , notificationRule(item.notificationRule)
    , chatStateSendingEnabled(item.chatStateSendingEnabled)
    , readMarkerSendingEnabled(item.readMarkerSendingEnabled)
    , pinningPosition(item.pinningPosition)
    , automaticMediaDownloadsRule(item.automaticMediaDownloadsRule)
{
}

std::variant<ClientRosterItemSettings, QXmppError> ClientRosterItemSettings::fromDom(const QDomElement &rootElement)
{
    Q_ASSERT(rootElement.tagName() == s_item);
    Q_ASSERT(rootElement.namespaceURI() == s_kaidan_export_ns);

    ClientRosterItemSettings data;

    if (const auto element = rootElement.firstChildElement(QStringLiteral("jid")); true) {
        data.jid = element.text();

        if (data.jid.isEmpty()) {
            return QXmppError{QStringLiteral("Missing required 'jid' element."), {}};
        }
    }

    readDataElement(encryption);
    readDataElement(notificationRule);
    readDataElement(chatStateSendingEnabled);
    readDataElement(readMarkerSendingEnabled);
    readDataElement(pinningPosition);
    readDataElement(automaticMediaDownloadsRule);

    return data;
}

void ClientRosterItemSettings::toXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(s_item.toString());
    writeDataElement(jid);
    writeDataElement(encryption);
    writeDataElement(notificationRule);
    writeDataElement(chatStateSendingEnabled);
    writeDataElement(readMarkerSendingEnabled);
    writeDataElement(pinningPosition);
    writeDataElement(automaticMediaDownloadsRule);
    writer.writeEndElement();
}

ClientConfiguration::ClientConfiguration(const AccountSettings *accountSettings)
    : jid(accountSettings->jid())
    , password(accountSettings->password())
    , host(accountSettings->host())
    , port(accountSettings->port())
    , tlsErrorsIgnored(accountSettings->tlsErrorsIgnored())
    , tlsRequirement(accountSettings->tlsRequirement())
    , passwordVisibility(accountSettings->passwordVisibility())
    , encryption(accountSettings->encryption())
    , automaticMediaDownloadsRule(accountSettings->automaticMediaDownloadsRule())
    , name(accountSettings->name())
    , contactNotificationRule(accountSettings->contactNotificationRule())
    , groupChatNotificationRule(accountSettings->groupChatNotificationRule())
    , geoLocationMapPreviewEnabled(accountSettings->geoLocationMapPreviewEnabled())
    , geoLocationMapService(accountSettings->geoLocationMapService())
{
}

std::variant<ClientConfiguration, QXmppError> ClientConfiguration::fromDom(const QDomElement &rootElement)
{
    Q_ASSERT(rootElement.tagName() == s_old_configuration);
    Q_ASSERT(rootElement.namespaceURI() == s_kaidan_export_ns);

    ClientConfiguration data;

    readDataElement(jid);
    readDataElement(password);
    readDataElement(host);
    readDataElement(port);
    readDataElement(tlsErrorsIgnored);
    readDataElement(tlsRequirement);
    readDataElement(passwordVisibility);
    readDataElement(encryption);
    readDataElement(automaticMediaDownloadsRule);
    readDataElement(name);
    readDataElement(contactNotificationRule);
    readDataElement(groupChatNotificationRule);
    readDataElement(geoLocationMapPreviewEnabled);
    readDataElement(geoLocationMapService);

    return data;
}

void ClientConfiguration::toXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(s_old_configuration.toString());
    writer.writeDefaultNamespace(s_kaidan_export_ns.toString());
    writeDataElement(jid);
    writeDataElement(password);
    writeDataElement(host);
    writeDataElement(port);
    writeDataElement(tlsErrorsIgnored);
    writeDataElement(tlsRequirement);
    writeDataElement(passwordVisibility);
    writeDataElement(encryption);
    writeDataElement(automaticMediaDownloadsRule);
    writeDataElement(name);
    writeDataElement(contactNotificationRule);
    writeDataElement(groupChatNotificationRule);
    writeDataElement(geoLocationMapPreviewEnabled);
    writeDataElement(geoLocationMapService);
    writer.writeEndElement();
}

QXmppConfiguration ClientConfiguration::toXmppConfiguration() const
{
    QXmppConfiguration configuration;

    if (jid) {
        configuration.setJid(*jid);
    }

    if (password) {
        configuration.setPassword(*password);
    }

    if (host) {
        configuration.setHost(*host);
    }

    if (port) {
        configuration.setPort(*port);
    }

    if (tlsErrorsIgnored) {
        configuration.setIgnoreSslErrors(*tlsErrorsIgnored);
    }

    if (tlsRequirement) {
        configuration.setStreamSecurityMode(*tlsRequirement);
    }

    return configuration;
}

ClientSettings::ClientSettings(AccountSettings *accountSettings, const QList<RosterItem> &rosterItems)
    : oldConfiguration(accountSettings)
    , roster(transform(rosterItems, [](const RosterItem &item) {
        return ClientRosterItemSettings(item);
    }))
{
}

QList<QString> ClientSettings::rosterContacts() const
{
    return transform<QList<QString>>(roster, [](const ClientRosterItemSettings &item) {
        return item.jid;
    });
}

std::variant<ClientSettings, QXmppError> ClientSettings::fromDom(const QDomElement &rootElement)
{
    Q_ASSERT(rootElement.tagName() == s_client_settings);
    Q_ASSERT(rootElement.namespaceURI() == s_kaidan_export_ns);

    const auto oldConfigurationElement = rootElement.firstChildElement(s_old_configuration.toString());
    const auto rosterElement = rootElement.firstChildElement(s_roster.toString());
    ClientSettings settings;

    if (oldConfigurationElement.namespaceURI() == s_kaidan_export_ns) {
        const auto result = ClientConfiguration::fromDom(oldConfigurationElement);

        if (const auto error = std::get_if<QXmppError>(&result)) {
            return *error;
        }

        settings.oldConfiguration = std::get<ClientConfiguration>(result);
    }

    if (rosterElement.namespaceURI() == s_kaidan_export_ns) {
        settings.roster.reserve(rosterElement.childNodes().size());

        for (QDomNode node = rosterElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
            if (!node.isElement()) {
                continue;
            }

            const auto element = node.toElement();

            if (element.tagName() == s_item) {
                const auto result = ClientRosterItemSettings::fromDom(element);

                if (const auto error = std::get_if<QXmppError>(&result)) {
                    return *error;
                }

                settings.roster.append(std::get<ClientRosterItemSettings>(result));
            }
        }
    }

    return settings;
}

void ClientSettings::toXml(QXmlStreamWriter &writer) const
{
    writer.writeStartElement(s_kaidan_export_ns.toString(), s_client_settings.toString());

    // Old configuration
    oldConfiguration.toXml(writer);

    // Roster items
    writer.writeStartElement(s_roster.toString());
    writer.writeDefaultNamespace(s_kaidan_export_ns.toString());
    for (const auto &entry : roster) {
        entry.toXml(writer);
    }
    writer.writeEndElement();

    writer.writeEndElement();
}
