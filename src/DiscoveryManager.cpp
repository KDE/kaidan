// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Nicolas Fella <nicolas.fella@gmx.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DiscoveryManager.h"
// QXmpp
#include <QXmppDiscoveryManager.h>
#include <QXmppDiscoveryIq.h>

DiscoveryManager::DiscoveryManager(QXmppClient *client, QObject *parent)
	: QObject(parent), m_client(client), m_manager(client->findExtension<QXmppDiscoveryManager>())
{
	// we're a normal client (not a server, gateway, server component, etc.)
	m_manager->setClientCategory("client");
	m_manager->setClientName(APPLICATION_DISPLAY_NAME);
#if defined Q_OS_ANDROID || defined UBUNTU_TOUCH
	// on Ubuntu Touch and Android we're always a mobile client
	m_manager->setClientType("phone");
#else
	// Plasma Mobile packages won't differ from desktop builds, so we need to check the mobile
	// variable on runtime.
	if (!qEnvironmentVariableIsEmpty("QT_QUICK_CONTROLS_MOBILE"))
		m_manager->setClientType("phone");
	else
		m_manager->setClientType("pc");
#endif

	connect(client, &QXmppClient::connected, this, &DiscoveryManager::handleConnection);
	connect(m_manager, &QXmppDiscoveryManager::itemsReceived,
	        this, &DiscoveryManager::handleItems);
}

DiscoveryManager::~DiscoveryManager()
{
}

void DiscoveryManager::handleConnection()
{
	// request disco info & items from the server
	// results are used by the QXmppUploadRequestManager
	m_manager->requestInfo(m_client->configuration().domain());
	m_manager->requestItems(m_client->configuration().domain());
}

void DiscoveryManager::handleItems(const QXmppDiscoveryIq &iq)
{
	// request info from all items
	const QList<QXmppDiscoveryIq::Item> items = iq.items();
	for (const QXmppDiscoveryIq::Item &item : items) {
		if (item.jid() == m_client->configuration().domain())
			continue;
		m_manager->requestInfo(item.jid());
	}
}
