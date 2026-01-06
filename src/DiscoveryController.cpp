// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DiscoveryController.h"

// QXmpp
#include <QXmppContactAddresses.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppTask.h>
#include <QXmppUri.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "KaidanCoreLog.h"

const auto CATEGORY = QStringLiteral("client");
const auto DESKTOP_TYPE = QStringLiteral("desktop");
const auto MOBILE_TYPE = QStringLiteral("mobile");

DiscoveryController::DiscoveryController(AccountSettings *accountSettings, Connection *connection, QXmppDiscoveryManager *discoveryManager, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_manager(discoveryManager)
{
    connect(connection, &Connection::connected, this, &DiscoveryController::updateData);

    QXmppDiscoIdentity identity;
    identity.setCategory(CATEGORY);
#if defined Q_OS_ANDROID
    // For systems such as Ubuntu Touch and Android, the type is determined by Kaidan's build.
    identity->setType(MOBILE_TYPE);
#else
    // For systems such as Plasma Mobile, the type is not determined by Kaidan's build.
    if (!qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_MOBILE")) {
        identity.setType(MOBILE_TYPE);
    } else {
        identity.setType(DESKTOP_TYPE);
    }
#endif
    identity.setName(QStringLiteral(APPLICATION_DISPLAY_NAME));

    m_manager->setIdentities({identity});
}

DiscoveryController::~DiscoveryController()
{
}

void DiscoveryController::updateData()
{
    const auto serverJid = QXmppUtils::jidToDomain(m_accountSettings->jid());

    m_manager->info(serverJid).then(this, [this, serverJid](QXmpp::Result<QXmppDiscoInfo> &&result) mutable {
        if (const auto *error = std::get_if<QXmppError>(&result)) {
            qCDebug(KAIDAN_CORE_LOG) << QStringLiteral("Could not retrieve discovery info for %1: %2").arg(serverJid, error->description);
        } else {
            handleOwnServerInfo(std::get<QXmppDiscoInfo>(std::move(result)));
        }
    });

    updateDataForManagers();
}

void DiscoveryController::updateDataForManagers()
{
    // Get info for the user JID.
    m_manager->requestInfo({});

    const auto serverJid = QXmppUtils::jidToDomain(m_accountSettings->jid());

    m_manager->requestInfo(serverJid);

    m_manager->requestDiscoItems(serverJid).then(this, [this, serverJid](QXmppDiscoveryManager::ItemsResult &&result) mutable {
        if (const auto *error = std::get_if<QXmppError>(&result)) {
            qCDebug(KAIDAN_CORE_LOG) << QStringLiteral("Could not retrieve discovery items of %1: %2").arg(serverJid, error->description);
        } else {
            const auto items = std::get<QList<QXmppDiscoveryIq::Item>>(std::move(result));

            // Get info of all child items.
            for (const auto &item : items) {
                m_manager->requestInfo(item.jid());
            }
        }
    });
}

void DiscoveryController::handleOwnServerInfo(QXmppDiscoInfo &&info)
{
    if (auto contactAddresses = info.dataForm<QXmppContactAddresses>()) {
        auto supportAddresses = contactAddresses->supportAddresses();

        m_accountSettings->setChatSupportAddresses(transformFilter(supportAddresses, [](const QString &supportAddress) -> std::optional<QString> {
            if (const auto uriParsingResult = QXmppUri::fromString(supportAddress); std::holds_alternative<QXmppUri>(uriParsingResult)) {
                if (const auto uri = std::get<QXmppUri>(uriParsingResult); !uri.query().has_value()) {
                    return uri.jid();
                }
            }

            return {};
        }));

        m_accountSettings->setGroupChatSupportAddresses(transformFilter(supportAddresses, [](const QString &supportAddress) -> std::optional<QString> {
            if (const auto uriParsingResult = QXmppUri::fromString(supportAddress); std::holds_alternative<QXmppUri>(uriParsingResult)) {
                if (const auto uri = std::get<QXmppUri>(uriParsingResult); uri.query().type() == typeid(QXmpp::Uri::Join)) {
                    return uri.jid();
                }
            }

            return {};
        }));
    }
}
