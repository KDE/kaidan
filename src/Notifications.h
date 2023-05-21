// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
