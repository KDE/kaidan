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

#pragma once

// Qt
#include <QObject>
// Kaidan
class AvatarFileStorage;
class Kaidan;
class RosterModel;
class VCardManager;
// QXmpp
class QXmppClient;
class QXmppMessage;
class QXmppRosterManager;

class RosterManager : public QObject
{
	Q_OBJECT

public:
	RosterManager(Kaidan *kaidan, QXmppClient *client, RosterModel *rosterModel,
				  AvatarFileStorage *avatarStorage, VCardManager *vCardManager,
	              QObject *parent = nullptr);

public slots:
	void addContact(const QString &jid, const QString &name, const QString &msg);
	void removeContact(const QString &jid);
	void renameContact(const QString &jid, const QString &newContactName);
	void handleSendMessage(const QString &jid, const QString &message,
	                       bool isSpoiler = false, const QString &spoilerHint = QString());

private slots:
	void populateRoster();
	void setCurrentChatJid(const QString &currentChatJid);
	void handleMessage(const QXmppMessage &msg);

private:
	Kaidan *m_kaidan;
	QXmppClient *m_client;
	RosterModel *m_model;
	AvatarFileStorage *m_avatarStorage;
	VCardManager *m_vCardManager;
	QXmppRosterManager *m_manager;
	QString m_currentChatJid;
};
