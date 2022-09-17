/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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

#include <QDateTime>
#include <QObject>
#include <QUuid>

class KNotification;
class QDateTime;
class QString;

class Notifications : public QObject
{
	Q_OBJECT

public:
	struct NotificationWrapper
	{
		QUuid id;
		QString accountJid;
		QString chatJid;
		QDateTime initalTimestamp;
		QString latestMessageId;
		QVector<QString> messages;
		bool isDeletionEnabled = true;
		KNotification *notification = nullptr;
	};

	static Notifications *instance();

	explicit Notifications(QObject *parent = nullptr);

	/**
	 * Sends a system notification for a chat message.
	 *
	 * @param accountJid JID of the message's account
	 * @param chatJid JID of the message's chat
	 * @param messageId ID of the message
	 * @param messageBody body of the message to display as the notification's text
	 */
	void sendMessageNotification(const QString &accountJid, const QString &chatJid, const QString &messageId, const QString &messageBody);

	void closeMessageNotification(const QString &accountJid, const QString &chatJid);

	/**
	 * Emitted to close all chat message notifications of the same age or older than a timestamp.
	 */
	Q_SIGNAL void closeMessageNotificationRequested(const QString &accountJid, const QString &chatJid);

private:
	QVector<NotificationWrapper> m_openNotifications;

	static Notifications *s_instance;
};
