// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMap>
#include <QObject>
// QXmpp
class QXmppClient;
class QXmppPresence;
class QXmppRosterManager;
// Kaidan
class AvatarFileStorage;
class ClientWorker;
class VCardManager;

class RosterManager : public QObject
{
	Q_OBJECT

public:
	RosterManager(ClientWorker *clientWorker, QXmppClient *client, QObject *parent = nullptr);

	void addContact(const QString &jid, const QString &name = {}, const QString &message = {}, bool automaticInitialAddition = false);
	void removeContact(const QString &jid);
	void renameContact(const QString &jid, const QString &newContactName);

	void subscribeToPresence(const QString &contactJid);
	Q_SIGNAL void presenceSubscriptionRequestReceived(const QString &accountJid, const QXmppPresence &request);
	bool acceptSubscriptionToPresence(const QString &contactJid);
	bool refuseSubscriptionToPresence(const QString &contactJid);
	QMap<QString, QXmppPresence> unrespondedPresenceSubscriptionRequests();

	void updateGroups(const QString &jid, const QString &name, const QVector<QString> &groups = {});
	Q_SIGNAL void updateGroupsRequested(const QString &jid, const QString &name, const QVector<QString> &groups);

Q_SIGNALS:
	/**
	 * Add a contact to your roster
	 *
	 * @param nick A simple nick name for the new contact, which should be
	 *             used to display in the roster.
	 * @param message message presented to the added contact
	 * @param automaticInitialAddition whether the contact is added after a first received message
	 */
	void addContactRequested(const QString &jid, const QString &nick = {}, const QString &message = {}, bool automaticInitialAddition = false);

	/**
	 * Remove a contact from your roster
	 *
	 * Only the JID is needed.
	 */
	void removeContactRequested(const QString &jid);

	/**
	 * Change a contact's name
	 */
	void renameContactRequested(const QString &jid, const QString &newContactName);

	void subscribeToPresenceRequested(const QString &contactJid);
	void acceptSubscriptionToPresenceRequested(const QString &contactJid);
	void refuseSubscriptionToPresenceRequested(const QString &contactJid);

private:
	void populateRoster();
	void handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
	void processSubscriptionRequestFromStranger(const QString &subscriberJid, const QXmppPresence &request);
	void addUnrespondedSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
	void applyOldContactData( const QString &oldContactJid, const QString &newContactJid);

	ClientWorker *m_clientWorker;
	QXmppClient *m_client;
	AvatarFileStorage *m_avatarStorage;
	VCardManager *m_vCardManager;
	QXmppRosterManager *m_manager;
	QMap<QString, QXmppPresence> m_unprocessedSubscriptionRequests;
	QMap<QString, QXmppPresence> m_pendingSubscriptionRequests;
	QMap<QString, QXmppPresence> m_unrespondedSubscriptionRequests;
	bool m_addingOwnJidToRosterAllowed = true;
	bool m_isItemBeingChanged = false;
};
