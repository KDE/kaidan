// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterManager.h"
// Kaidan
#include "AvatarFileStorage.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "Notifications.h"
#include "OmemoManager.h"
#include "RosterModel.h"
#include "Settings.h"
#include "VCardManager.h"
// QXmpp
#include <QXmppRosterManager.h>
#include <QXmppTask.h>

RosterManager::RosterManager(ClientWorker *clientWorker,
                             QXmppClient *client,
                             QObject *parent)
	: QObject(parent),
	  m_clientWorker(clientWorker),
	  m_client(client),
	  m_avatarStorage(clientWorker->caches()->avatarStorage),
	  m_vCardManager(clientWorker->vCardManager()),
	  m_manager(client->findExtension<QXmppRosterManager>())
{
	connect(m_manager, &QXmppRosterManager::rosterReceived, this, &RosterManager::populateRoster);

	connect(m_manager, &QXmppRosterManager::itemAdded,
		this, [this](const QString &jid) {
		RosterItem rosterItem { m_client->configuration().jidBare(), m_manager->getRosterEntry(jid) };
		rosterItem.encryption = Kaidan::instance()->settings()->encryption();
		rosterItem.automaticMediaDownloadsRule = RosterItem::AutomaticMediaDownloadsRule::Default;
		rosterItem.lastMessageDateTime = QDateTime::currentDateTimeUtc();
		Q_EMIT RosterModel::instance()->addItemRequested(rosterItem);

		if (m_client->state() == QXmppClient::ConnectedState) {
			m_vCardManager->requestVCard(jid);
		}
	});

	connect(m_manager, &QXmppRosterManager::itemChanged,
		this, [this] (const QString &jid) {
		Q_EMIT RosterModel::instance()->updateItemRequested(jid, [this, jid](RosterItem &item) {
			const auto updatedItem = m_manager->getRosterEntry(jid);
			item.name = updatedItem.name();
			item.subscription = updatedItem.subscriptionType();

			const auto groups = updatedItem.groups();
			item.groups = QVector(groups.cbegin(), groups.cend());
		});

		if (m_isItemBeingChanged) {
			m_clientWorker->finishTask();
			m_isItemBeingChanged = false;
		}
	});

	connect(m_manager, &QXmppRosterManager::itemRemoved, this, [this](const QString &jid) {
		const auto accountJid = m_client->configuration().jidBare();
		MessageDb::instance()->removeAllMessagesFromChat(accountJid, jid);
		Q_EMIT RosterModel::instance()->removeItemsRequested(accountJid, jid);
		m_clientWorker->omemoManager()->removeContactDevices(jid);
	});

	connect(m_manager, &QXmppRosterManager::subscriptionRequestReceived, this, &RosterManager::handleSubscriptionRequest);

	// user actions
	connect(this, &RosterManager::addContactRequested, this, &RosterManager::addContact);
	connect(this, &RosterManager::removeContactRequested, this, &RosterManager::removeContact);
	connect(this, &RosterManager::renameContactRequested, this, &RosterManager::renameContact);

	connect(this, &RosterManager::subscribeToPresenceRequested, this, &RosterManager::subscribeToPresence);
	connect(this, &RosterManager::acceptSubscriptionToPresenceRequested, this, &RosterManager::acceptSubscriptionToPresence);
	connect(this, &RosterManager::refuseSubscriptionToPresenceRequested, this, &RosterManager::refuseSubscriptionToPresence);

	connect(this, &RosterManager::updateGroupsRequested, this, &RosterManager::updateGroups);
}

void RosterManager::populateRoster()
{
	qDebug() << "[client] [RosterManager] Populating roster";

	// create a new list of contacts
	QHash<QString, RosterItem> items;
	const QStringList bareJids = m_manager->getRosterBareJids();

	for (const auto &jid : bareJids) {
		RosterItem rosterItem { m_client->configuration().jidBare(), m_manager->getRosterEntry(jid)};
		rosterItem.encryption = Kaidan::instance()->settings()->encryption();
		rosterItem.automaticMediaDownloadsRule = RosterItem::AutomaticMediaDownloadsRule::Default;
		items.insert(jid, rosterItem);

		if (m_avatarStorage->getHashOfJid(jid).isEmpty() && m_client->state() == QXmppClient::ConnectedState) {
			m_vCardManager->requestVCard(jid);
		}

		// Remove subscription requests from JIDs that are in the roster.
		m_unprocessedSubscriptions.remove(jid);
	}

	// replace current contacts with new ones from server
	Q_EMIT RosterModel::instance()->replaceItemsRequested(items);

	// Process subscription requests that were received before the roster was received.
	for (auto itr = m_unprocessedSubscriptions.begin(); itr != m_unprocessedSubscriptions.end();) {
		processSubscriptionRequest(itr.key(), itr.value());
		itr = m_unprocessedSubscriptions.erase(itr);
	}
}

void RosterManager::handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &presence)
{
	const auto requestText = presence.statusText();

	if (m_manager->isRosterReceived()) {
		if (!m_manager->getRosterBareJids().contains(subscriberJid)) {
			processSubscriptionRequest(subscriberJid, requestText);
		}
	} else {
		m_unprocessedSubscriptions.insert(subscriberJid, requestText);
	}
}

void RosterManager::processSubscriptionRequest(const QString &subscriberJid, const QString &requestText)
{
	addContact(subscriberJid);

	const auto accountJid = m_client->configuration().jidBare();

	Message message;
	message.accountJid = accountJid;
	message.chatJid = subscriberJid;
	message.senderId = subscriberJid;
	message.body = requestText;

	if (!requestText.isEmpty()) {
		MessageDb::instance()->addMessage(message, MessageOrigin::Stream);
	} else {
		runOnThread(Notifications::instance(), [accountJid, subscriberJid]() {
			Notifications::instance()->sendMessageNotification(accountJid, subscriberJid, {}, tr("New contact"));
		});
	}
}

void RosterManager::addContact(const QString &jid, const QString &name, const QString &msg)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		m_manager->addItem(jid, name);
		m_manager->subscribe(jid, msg);
	} else {
		Q_EMIT Kaidan::instance()->passiveNotificationRequested(
			tr("Could not add contact, as a result of not being connected.")
		);
		qWarning() << "[client] [RosterManager] Could not add contact, as a result of "
		              "not being connected.";
	}
}

void RosterManager::removeContact(const QString &jid)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		m_manager->removeItem(jid);
	} else {
		Q_EMIT Kaidan::instance()->passiveNotificationRequested(
			tr("Could not remove contact, as a result of not being connected.")
		);
		qWarning() << "[client] [RosterManager] Could not remove contact, as a result of "
		              "not being connected.";
	}
}

void RosterManager::renameContact(const QString &jid, const QString &newContactName)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		m_manager->renameItem(jid, newContactName);
	} else {
		Q_EMIT Kaidan::instance()->passiveNotificationRequested(
			tr("Could not rename contact, as a result of not being connected.")
		);
		qWarning() << "[client] [RosterManager] Could not rename contact, as a result of "
		              "not being connected.";
	}
}

void RosterManager::subscribeToPresence(const QString &contactJid)
{
	m_manager->subscribeTo(contactJid).then(this, [contactJid](QXmpp::SendResult result) {
		if (const auto error = std::get_if<QXmppError>(&result)) {
			Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Requesting to see the status of %1 failed because of a connection problem: %2").arg(contactJid, error->description));
		}
	});
}

void RosterManager::acceptSubscriptionToPresence(const QString &contactJid)
{
	if (!m_manager->acceptSubscription(contactJid)) {
		Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Allowing %1 to see your status failed").arg(contactJid));
	}
}

void RosterManager::refuseSubscriptionToPresence(const QString &contactJid)
{
	if (!m_manager->refuseSubscription(contactJid)) {
		Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Disallowing %1 to see your status failed").arg(contactJid));
	}
}

void RosterManager::updateGroups(const QString &jid, const QString &name, const QVector<QString> &groups)
{
	m_isItemBeingChanged = true;

	m_clientWorker->startTask(
		[this, jid, name, groups] {
			// TODO: Add updating only groups to QXmppRosterManager without the need to pass the unmodified name
			m_manager->addItem(jid, name, QSet(groups.cbegin(), groups.cend()));
		}
	);
}
