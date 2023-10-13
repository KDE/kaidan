// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Andrea Scarpino <scarpino@kde.org>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VCardManager.h"

// Qt
#include <QBuffer>
// QXmpp
#include <QXmppUtils.h>
#include <QXmppVCardManager.h>
#include <QXmppVCardIq.h>
// Kaidan
#include "AvatarFileStorage.h"
#include "Kaidan.h"
#include "VCardCache.h"

VCardManager::VCardManager(ClientWorker *clientWorker, QXmppClient *client, AvatarFileStorage *avatars, QObject *parent)
	: QObject(parent), m_clientWorker(clientWorker), m_client(client), m_manager(client->findExtension<QXmppVCardManager>()), m_avatarStorage(avatars)
{
	connect(m_manager, &QXmppVCardManager::vCardReceived, this, &VCardManager::handleVCardReceived);
	connect(m_client, &QXmppClient::presenceReceived, this, &VCardManager::handlePresenceReceived);
	connect(m_manager, &QXmppVCardManager::clientVCardReceived, this, &VCardManager::handleClientVCardReceived);
	connect(this, &VCardManager::vCardRequested, this, &VCardManager::requestVCard);
	connect(this, &VCardManager::clientVCardRequested, this, &VCardManager::requestClientVCard);
	connect(this, &VCardManager::changeNicknameRequested, this, &VCardManager::changeNickname);
	connect(this, &VCardManager::changeAvatarRequested, this, &VCardManager::changeAvatar);

	// Currently we're not requesting the own VCard on every connection because it is probably
	// way too resource intensive on mobile connections with many reconnects.
	// Actually we would need to request our own avatar, calculate the hash of it and publish
	// that in our presence.
	//
	// XEP-0084: User Avatar - probably best option (as long as the servers support XEP-0398:
	//                         User Avatar to vCard-Based Avatars Conversion)
}

void VCardManager::requestVCard(const QString &jid)
{
	if (m_client->state() == QXmppClient::ConnectedState)
		m_manager->requestVCard(jid);
	else
		qWarning() << "[VCardManager] Could not fetch vCard: Not connected to a server";
}

void VCardManager::handleVCardReceived(const QXmppVCardIq &iq)
{
	if (!iq.photo().isEmpty()) {
		m_avatarStorage->addAvatar(QXmppUtils::jidToBareJid(iq.from().isEmpty() ? m_client->configuration().jid() : iq.from()), iq.photo());
	}

	Q_EMIT vCardReceived(iq);
}

void VCardManager::requestClientVCard()
{
	if (m_client->state() == QXmppClient::ConnectedState)
		m_manager->requestClientVCard();
	else
		qWarning() << "[VCardManager] Could not fetch own vCard: Not connected to a server";
}

void VCardManager::handleClientVCardReceived()
{
	if (!m_nicknameToBeSetAfterReceivingCurrentVCard.isNull()) {
		changeNicknameAfterReceivingCurrentVCard();
	}

	if (!m_avatarToBeSetAfterReceivingCurrentVCard.isNull() || m_isAvatarToBeReset) {
		changeAvatarAfterReceivingCurrentVCard();
	}

	const auto &ownJid { m_client->configuration().jidBare() };
	auto clientVCard { m_manager->clientVCard() };
	clientVCard.setFrom(ownJid);

	m_clientWorker->caches()->vCardCache->setVCard(ownJid, clientVCard);
}

void VCardManager::handlePresenceReceived(const QXmppPresence &presence)
{
	if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateValidPhoto) {
		QString hash = m_avatarStorage->getHashOfJid(QXmppUtils::jidToBareJid(presence.from()));
		QString newHash = presence.photoHash().toHex();

		// check if hash differs and we need to refetch the avatar
		if (hash != newHash)
			m_manager->requestVCard(QXmppUtils::jidToBareJid(presence.from()));

	} else if (presence.vCardUpdateType() == QXmppPresence::VCardUpdateNoPhoto) {
		QString bareJid = QXmppUtils::jidToBareJid(presence.from());
		m_avatarStorage->clearAvatar(bareJid);
	}
	// ignore VCardUpdateNone (protocol unsupported) and VCardUpdateNotReady
}

void VCardManager::changeNickname(const QString &nickname)
{
	m_clientWorker->startTask(
		[this, nickname] {
			m_nicknameToBeSetAfterReceivingCurrentVCard = nickname;
			requestClientVCard();
		}
	);
}

void VCardManager::changeAvatar(const QImage &avatar)
{
	m_clientWorker->startTask(
		[this, avatar] {
			// TODO what's the maximum image size that should be saved?
			if (!avatar.isNull()) {
				m_avatarToBeSetAfterReceivingCurrentVCard = avatar.scaledToWidth(512);
			} else {
				m_isAvatarToBeReset = true;
			}

			requestClientVCard();
		}
	);
}

void VCardManager::changeNicknameAfterReceivingCurrentVCard()
{
	QXmppVCardIq vCardIq = m_manager->clientVCard();
	vCardIq.setNickName(m_nicknameToBeSetAfterReceivingCurrentVCard);
	m_manager->setClientVCard(vCardIq);
	m_nicknameToBeSetAfterReceivingCurrentVCard.clear();
	m_clientWorker->finishTask();
}

void VCardManager::changeAvatarAfterReceivingCurrentVCard()
{
	QXmppVCardIq vCardIq = m_manager->clientVCard();

	if (!m_isAvatarToBeReset) {
		QByteArray ba;
		QBuffer buffer(&ba);
		buffer.open(QIODevice::WriteOnly);
		m_avatarToBeSetAfterReceivingCurrentVCard.save(&buffer, "JPG");

		vCardIq.setPhoto(ba);
	} else {
		m_isAvatarToBeReset = false;
		vCardIq.setPhoto({});
	}

	m_manager->setClientVCard(vCardIq);

	m_avatarToBeSetAfterReceivingCurrentVCard = {};
	m_clientWorker->finishTask();

	Q_EMIT Kaidan::instance()->avatarChangeSucceeded();
}
