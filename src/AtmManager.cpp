// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AtmManager.h"

#include <QXmppAtmManager.h>
#include <QXmppClient.h>

#include "qxmpp-exts/QXmppUri.h"

#include "TrustDb.h"

AtmManager::AtmManager(QXmppClient *client, Database *database, QObject *parent)
	: QObject(parent),
	  m_trustStorage(new TrustDb(database, this, {}, this)),
	  m_manager(client->addNewExtension<QXmppAtmManager>(m_trustStorage.get()))
{
}

AtmManager::~AtmManager() = default;

void AtmManager::setAccountJid(const QString &accountJid)
{
	m_trustStorage->setAccountJid(accountJid);
}

void AtmManager::makeTrustDecisions(const QXmppUri &uri)
{
	QList<QByteArray> keyIdsForAuthentication;
	const auto trustedKeysIds = uri.trustedKeysIds();
	for (const auto &keyId : trustedKeysIds) {
		keyIdsForAuthentication.append(QByteArray::fromHex(keyId.toUtf8()));
	}

	QList<QByteArray> keyIdsForDistrusting;
	const auto distrustedKeysIds = uri.distrustedKeysIds();
	for (const auto &keyId : distrustedKeysIds) {
		keyIdsForDistrusting.append(QByteArray::fromHex(keyId.toUtf8()));
	}

	m_manager->makeTrustDecisions(uri.encryption(), uri.jid(), keyIdsForAuthentication, keyIdsForDistrusting);
}
