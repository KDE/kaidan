/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2020 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef VCARDMANAGER_H
#define VCARDMANAGER_H

#include <QObject>
#include <QString>
#include <QXmppVCardManager.h>
#include <QXmppPresence.h>

class AvatarFileStorage;
class QXmppClient;

class VCardManager : public QObject
{
	Q_OBJECT

public:
	VCardManager(QXmppClient *client, AvatarFileStorage *avatars, QObject *parent = nullptr);

	/**
	 * Will request the VCard from the server
	 */
	void fetchVCard(const QString& jid);

	/**
	 * Handles incoming VCards and processes them (save avatar, etc.)
	 */
	void handleVCard(const QXmppVCardIq &iq);

	/**
	 * Requests the user's vCard from the server.
	 */
	void fetchClientVCard();

	/**
	 * Handles the receiving of the user's vCard.
	 */
	void handleClientVCardReceived();

	/**
	 * Handles incoming presences and checks if the avatar needs to be refreshed
	 */
	void handlePresence(const QXmppPresence &presence);

	/**
	 * Updates the user's nickname.
	 *
	 * @param nickname name that is shown to contacts after the update
	 */
	void updateNickname(const QString &nickname);

signals:
	void vCardReceived(const QXmppVCardIq &vCard);

private:
	/**
	 * Updates the nickname which was set to be updated after fetching the current vCard.
	 */
	void updateNicknameAfterFetchingCurrentVCard();

	QXmppClient *client;
	QXmppVCardManager *manager;
	AvatarFileStorage *avatarStorage;
	QString m_nicknameToBeSetAfterFetchingCurrentVCard;
};

#endif // VCARDMANAGER_H
