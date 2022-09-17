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

#include "Notifications.h"

// KNotifications
#ifdef HAVE_KNOTIFICATIONS
#include <KNotification>
#endif

// Kaidan
#include "Kaidan.h"
#include "MessageHandler.h"
#include "RosterModel.h"

#include <QStringBuilder>
#include <QUuid>

// Q_OS_BSD4 includes all BSD variants and also Q_OS_DARWIN
// Q_OS_LINUX is also defined on Android
#if (defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_HURD)) && \
	!defined(Q_OS_ANDROID) && !defined(Q_OS_DARWIN)
#define DESKTOP_LINUX_ALIKE_OS
#endif

using namespace std::chrono_literals;

// Event ID corresponding to the section entry in the "kaidan.notifyrc" configuration file
constexpr QStringView NEW_MESSAGE_EVENT_ID = u"new-message";

constexpr int MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT = 6;

Notifications *Notifications::s_instance = nullptr;

Notifications *Notifications::instance()
{
	return s_instance;
}

Notifications::Notifications(QObject *parent)
	: QObject(parent)
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	connect(this, &Notifications::closeMessageNotificationRequested, this, &Notifications::closeMessageNotification);
}

#ifdef HAVE_KNOTIFICATIONS
void Notifications::sendMessageNotification(const QString &accountJid, const QString &chatJid, const QString &messageId, const QString &messageBody)
{
#ifdef DESKTOP_LINUX_ALIKE_OS
	static bool IS_USING_GNOME = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("GNOME", Qt::CaseInsensitive);
#endif

	KNotification *notification = nullptr;
	QUuid notificationId;

	auto notificationWrapperItr = std::find_if(m_openNotifications.begin(), m_openNotifications.end(), [&accountJid, &chatJid](const auto &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	// Update an existing notification or create a new one.
	if (notificationWrapperItr != m_openNotifications.end()) {
		auto &messages = notificationWrapperItr->messages;
		messages.append(messageBody);

		// Initialize variables by known values.
		notificationId = notificationWrapperItr->id;
		notificationWrapperItr->latestMessageId = messageId;

		QList<QString> notificationTextLines;

		// Append the message's body to the text of existing notifications.
		// If the text of the existing notifications and messageBody contain together more than
		// MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT of lines, keep only the last
		// MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT - 1 of them and replace the first one by an
		// ellipse.
		//
		// The loop exits in the following cases:
		// 1. The message of the current iteration has lines that would result in more
		// lines than MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT when prepended to
		// notificationTextLines.
		// 2. notificationTextLines would have more lines than
		// MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT when the next message was being prepended.
		for (auto messageItr = messages.end() - 1; messageItr != messages.begin() - 1; --messageItr) {
			auto messageNotificationTextLines = (*messageItr).split(u'\n');
			const auto overflowingMessageLineCount = messageNotificationTextLines.size() + notificationTextLines.size() - MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT;

			if (overflowingMessageLineCount > 0) {
				messageNotificationTextLines = messageNotificationTextLines.mid(overflowingMessageLineCount + 1);
				messageNotificationTextLines.prepend(QStringLiteral("…"));

				*messageItr = messageNotificationTextLines.join(u'\n');
				messages.erase(messages.begin(), messageItr);

				notificationTextLines = messageNotificationTextLines << notificationTextLines;
				break;
			} else {
				notificationTextLines = messageNotificationTextLines << notificationTextLines;

				if (notificationTextLines.size() ==  MAXIMUM_NOTIFICATION_TEXT_LINE_COUNT && messageItr - 1 != messages.begin() - 1) {
					notificationTextLines[0] = QStringLiteral("…");
					messages.erase(messages.begin(), messageItr);
					break;
				}
			}
		}

		notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
		notification->setText(notificationTextLines.join(u'\n'));

		notificationWrapperItr->isDeletionEnabled = false;
		notificationWrapperItr->notification->close();
		notificationWrapperItr->notification = notification;
	} else {
		notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
		notificationId = QUuid::createUuid();
		notification->setText(messageBody);

		NotificationWrapper notificationWrapper {
			.id = notificationId,
			.accountJid = accountJid,
			.chatJid = chatJid,
			.latestMessageId = messageId,
			.messages = { messageBody },
			.notification = notification
		};
		m_openNotifications.append(notificationWrapper);
	}

	// Use bare JID for users that are not present in our roster, so foreign users can't choose a
	// name that looks like a known contact.
	auto rosterItem = RosterModel::instance()->findItem(chatJid);
	auto chatName = rosterItem ? rosterItem->displayName() : chatJid;
	notification->setTitle(chatName);

#ifdef DESKTOP_LINUX_ALIKE_OS
	if (IS_USING_GNOME) {
		notification->setFlags(KNotification::Persistent);
	}
#endif
#ifdef Q_OS_ANDROID
	notification->setIconName("kaidan-bw");
#endif
	notification->setDefaultAction("Open");
	notification->setActions({
		QObject::tr("Mark as read")
	});

	QObject::connect(notification, &KNotification::defaultActivated, this, [=] {
		emit Kaidan::instance()->openChatPageRequested(accountJid, chatJid);
		emit Kaidan::instance()->raiseWindowRequested();
	});
	QObject::connect(notification, &KNotification::action1Activated, this, [=] {
		emit RosterModel::instance()->updateItemRequested(chatJid, [=](RosterItem &item) {
			item.lastReadContactMessageId = messageId;
			item.unreadMessages = 0;
		});
		emit Kaidan::instance()->client()->messageHandler()->sendReadMarkerRequested(chatJid, messageId);
	});

	QObject::connect(notification, &KNotification::closed, this, [=, this]() {
		auto notificationWrapperItr = std::find_if(m_openNotifications.begin(), m_openNotifications.end(), [accountJid, chatJid](const NotificationWrapper &notificationWrapper) {
			return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
		});

		if (notificationWrapperItr != m_openNotifications.end()) {
			if (notificationWrapperItr->isDeletionEnabled) {
				m_openNotifications.erase(notificationWrapperItr);
			} else {
				notificationWrapperItr->isDeletionEnabled = true;
			}
		}
	});

	notification->sendEvent();
}

void Notifications::closeMessageNotification(const QString &accountJid, const QString &chatJid)
{
	auto notificationWrapperItr = std::find_if(m_openNotifications.begin(), m_openNotifications.end(), [accountJid, chatJid](const NotificationWrapper &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	if (notificationWrapperItr != m_openNotifications.end()) {
		m_openNotifications.erase(notificationWrapperItr);
	}
}
#else
void Notifications::sendMessageNotification(const QString &, const QString &, const QString &, const QString &)
{
}

void Notifications::closeMessageNotification(const QString &, const QString &)
{
}
#endif // HAVE_KNOTIFICATIONS
