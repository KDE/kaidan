// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QImage>

class AvatarFileStorage;
class ClientWorker;
class QXmppClient;
class QXmppPresence;
class QXmppVCardIq;
class QXmppVCardManager;

class VCardManager : public QObject
{
	Q_OBJECT

public:
	VCardManager(ClientWorker *clientWorker, QXmppClient *client, AvatarFileStorage *avatars, QObject *parent = nullptr);

	/**
	 * Requests the vCard of a given JID from the JID's server.
	 *
	 * @param jid JID for which the vCard is being requested
	 */
	void requestVCard(const QString &jid);

	/**
	 * Handles an incoming vCard and processes it like saving a containing user avatar etc..
	 *
	 * @param iq IQ stanza containing the vCard
	 */
	void handleVCardReceived(const QXmppVCardIq &iq);

	/**
	 * Requests the user's vCard from the server.
	 */
	void requestClientVCard();

	/**
	 * Handles the receiving of the user's vCard.
	 */
	void handleClientVCardReceived();

	/**
	 * Handles an incoming presence stanza and checks if the user avatar needs to be refreshed.
	 *
	 * @param presence a contact's presence stanza
	 */
	void handlePresenceReceived(const QXmppPresence &presence);

	/**
	 * Executes a pending nickname change if the nickname could not be changed on the
	 * server before because the client was disconnected.
	 *
	 * @return true if the pending nickname change is executed on the second login with
	 * the same credentials or later, otherwise false
	 */
	bool executePendingNicknameChange();

	/**
	 * Executes a pending avatar change if the avatar could not be changed on the
	 * server before because the client was disconnected.
	 *
	 * @return true if the pending avatar change is executed on the second login with
	 * the same credentials or later, otherwise false
	 */
	bool executePendingAvatarChange();

signals:
	/**
	 * Emitted when any vCard is received.
	 *
	 * @param vCard received vCard
	 */
	void vCardReceived(const QXmppVCardIq &vCard);

	void vCardRequested(const QString &jid);
	void clientVCardRequested();
	void changeNicknameRequested(const QString &nickname);
	void changeAvatarRequested(const QImage &avatar = {});

private:
	/**
	 * Changes the user's nickname.
	 *
	 * @param nickname name that is shown to contacts after the update
	 */
	void changeNickname(const QString &nickname);
	void changeAvatar(const QImage &avatar = {});

	/**
	 * Changes the nickname which was cached to be set after receiving the current vCard.
	 */
	void changeNicknameAfterReceivingCurrentVCard();
	/**
	 * Changes the avatar which was cached to be set after receiving the current vCard.
	 */
	void changeAvatarAfterReceivingCurrentVCard();

	ClientWorker *m_clientWorker;
	QXmppClient *m_client;
	QXmppVCardManager *m_manager;
	AvatarFileStorage *m_avatarStorage;
	QString m_nicknameToBeSetAfterReceivingCurrentVCard;
	QImage m_avatarToBeSetAfterReceivingCurrentVCard;
	bool m_isAvatarToBeReset = false;
};
