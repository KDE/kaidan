// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DiscoveryManager.h"
// QXmpp
#include <QXmppDiscoveryManager.h>
#include <QXmppDiscoveryIq.h>
#include <QXmppTask.h>
// Kaidan
#include "Globals.h"

DiscoveryManager::DiscoveryManager(QXmppClient *client, QObject *parent)
	: QObject(parent), m_client(client), m_manager(client->findExtension<QXmppDiscoveryManager>())
{
	// we're a normal client (not a server, gateway, server component, etc.)
	m_manager->setClientCategory(QStringLiteral("client"));
	m_manager->setClientName(QStringLiteral(APPLICATION_DISPLAY_NAME));
#if defined Q_OS_ANDROID || defined UBUNTU_TOUCH
	// on Ubuntu Touch and Android we're always a mobile client
	m_manager->setClientType("phone");
#else
	// Plasma Mobile packages won't differ from desktop builds, so we need to check the mobile
	// variable on runtime.
	if (!qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_MOBILE"))
		m_manager->setClientType(QStringLiteral("phone"));
	else
		m_manager->setClientType(QStringLiteral("pc"));
#endif

	connect(client, &QXmppClient::connected, this, &DiscoveryManager::requestData);
}

DiscoveryManager::~DiscoveryManager()
{
}

void DiscoveryManager::requestData()
{
	// Request disco info for the user JID.
	m_manager->requestInfo({});

	const auto serverJid = m_client->configuration().domain();

	// Request disco info and items for the server JID.
	m_manager->requestInfo(serverJid);
	m_manager->requestDiscoItems(serverJid).then(this, [this, serverJid](QXmppDiscoveryManager::ItemsResult &&result) {
		if (const auto *error = std::get_if<QXmppError>(&result)) {
			qDebug() << QStringLiteral("Could not retrieve discovery items from %1: %2").arg(serverJid, error->description);
		} else {
			handleItems(std::get<QList<QXmppDiscoveryIq::Item>>(std::move(result)));
		}
	});
}

void DiscoveryManager::handleItems(QList<QXmppDiscoveryIq::Item> &&items)
{
	// request info from all items
	for (const auto &item : std::as_const(items)) {
		m_manager->requestInfo(item.jid());
	}
}
