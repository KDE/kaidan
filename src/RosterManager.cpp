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
#include "MessageModel.h"
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
	connect(m_manager, &QXmppRosterManager::rosterReceived,
	        this, &RosterManager::populateRoster);

	connect(m_manager, &QXmppRosterManager::itemAdded,
		this, [this](const QString &jid) {
		RosterItem rosterItem { m_client->configuration().jidBare(), m_manager->getRosterEntry(jid) };
		rosterItem.encryption = Kaidan::instance()->settings()->encryption();
		emit RosterModel::instance()->addItemRequested(rosterItem);

		if (m_client->state() == QXmppClient::ConnectedState) {
			m_vCardManager->requestVCard(jid);
		}
	});

	connect(m_manager, &QXmppRosterManager::itemChanged,
		this, [this] (const QString &jid) {
		emit RosterModel::instance()->updateItemRequested(jid, [this, jid](RosterItem &item) {
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
		emit MessageModel::instance()->removeMessagesRequested(accountJid, jid);
		emit RosterModel::instance()->removeItemsRequested(accountJid, jid);
		m_clientWorker->omemoManager()->removeContactDevices(jid);
	});

	connect(m_manager, &QXmppRosterManager::subscriptionRequestReceived,
	        this, [](const QString &subscriberBareJid, const QXmppPresence &presence) {
		emit RosterModel::instance()->subscriptionRequestReceived(subscriberBareJid, presence.statusText());
	});
	connect(this, &RosterManager::answerSubscriptionRequestRequested,
	        this, [this](QString jid, bool accepted) {
		if (accepted) {
			m_manager->acceptSubscription(jid);

			// do not send a subscription request if both users have already subscribed
			// each others presence
			if (m_manager->getRosterEntry(jid).subscriptionType() != QXmppRosterIq::Item::Both)
				m_manager->subscribe(jid);
		} else {
			m_manager->refuseSubscription(jid);
		}
	});

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
	const auto initialTime = QDateTime::currentDateTimeUtc();
	for (const auto &jid : bareJids) {
		RosterItem rosterItem { m_client->configuration().jidBare(), m_manager->getRosterEntry(jid), initialTime };
		rosterItem.encryption = Kaidan::instance()->settings()->encryption();
		items.insert(jid, rosterItem);

		if (m_avatarStorage->getHashOfJid(jid).isEmpty() && m_client->state() == QXmppClient::ConnectedState) {
			m_vCardManager->requestVCard(jid);
		}
	}

	// replace current contacts with new ones from server
	emit RosterModel::instance()->replaceItemsRequested(items);
}

void RosterManager::addContact(const QString &jid, const QString &name, const QString &msg)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		m_manager->addItem(jid, name);
		m_manager->subscribe(jid, msg);
	} else {
		emit Kaidan::instance()->passiveNotificationRequested(
			tr("Could not add contact, as a result of not being connected.")
		);
		qWarning() << "[client] [RosterManager] Could not add contact, as a result of "
		              "not being connected.";
	}
}

void RosterManager::removeContact(const QString &jid)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		m_manager->unsubscribe(jid);
		m_manager->removeItem(jid);
	} else {
		emit Kaidan::instance()->passiveNotificationRequested(
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
		emit Kaidan::instance()->passiveNotificationRequested(
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
			emit Kaidan::instance()->passiveNotificationRequested(tr("Requesting to see the status of %1 failed because of a connection problem: %2").arg(contactJid, error->description));
		}
	});
}

void RosterManager::acceptSubscriptionToPresence(const QString &contactJid)
{
	if (!m_manager->acceptSubscription(contactJid)) {
		emit Kaidan::instance()->passiveNotificationRequested(tr("Allowing %1 to see your status failed").arg(contactJid));
	}
}

void RosterManager::refuseSubscriptionToPresence(const QString &contactJid)
{
	if (!m_manager->refuseSubscription(contactJid)) {
		emit Kaidan::instance()->passiveNotificationRequested(tr("Disallowing %1 to see your status failed").arg(contactJid));
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
