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

#include <QUuid>

// Q_OS_BSD4 includes all BSD variants and also Q_OS_DARWIN
// Q_OS_LINUX is also defined on Android
#if (defined(Q_OS_LINUX) || defined(Q_OS_BSD4) || defined(Q_OS_HURD)) && \
	!defined(Q_OS_ANDROID) && !defined(Q_OS_DARWIN)
#define DESKTOP_LINUX_ALIKE_OS
#endif

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

	connect(this, &Notifications::closeMessageNotificationsRequested, this, &Notifications::closeMessageNotifications);
}

#ifdef HAVE_KNOTIFICATIONS

void Notifications::sendMessageNotification(const QString &accountJid, const QString &chatJid, const QString &messageId, const QDateTime &timestamp, const QString &messageBody)
{
#ifdef DESKTOP_LINUX_ALIKE_OS
	static bool IS_USING_GNOME = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("GNOME", Qt::CaseInsensitive);
#endif

	// Use bare JID for users that are not present in our roster, so foreign users can't choose a
	// name that looks like a known contact.
	auto rosterItem = RosterModel::instance()->findItem(chatJid);
	auto chatName = rosterItem ? rosterItem->displayName() : chatJid;

	auto *notification = new KNotification("new-message");
	notification->setTitle(chatName);
	notification->setText(messageBody);
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

	const auto notificationId = QUuid::createUuid();
	const auto isPersistentNotification = notification->flags().testFlag(KNotification::Persistent);

	NotificationWrapper notificationWrapper {
		.id = notificationId,
		.accountJid = accountJid,
		.chatJid = chatJid,
		.timestamp = timestamp,
		.notification = notification
	};
	m_openNotifications.append(notificationWrapper);

	QObject::connect(notification, &KNotification::defaultActivated, this, [=, this] {
		disableResending(isPersistentNotification, notificationId);

		emit Kaidan::instance()->openChatPageRequested(accountJid, chatJid);
		emit Kaidan::instance()->raiseWindowRequested();
	});
	QObject::connect(notification, &KNotification::action1Activated, this, [=, this] {
		disableResending(isPersistentNotification, notificationId);
		closeMessageNotifications(accountJid, chatJid, timestamp, notificationId);

		emit RosterModel::instance()->updateItemRequested(chatJid, [](RosterItem &item) {
			item.unreadMessages = item.unreadMessages - 1;
		});
		emit Kaidan::instance()->client()->messageHandler()->sendReadMarkerRequested(chatJid, messageId);
	});

	QObject::connect(notification, &KNotification::closed, this, [=, this]() {
		for (auto itr = m_openNotifications.begin(); itr != m_openNotifications.end(); ++itr) {
			if (itr->id == notificationId) {
				// Send a message notification again if the notification is persistent but
				// nevertheless closed by the operating system.
				// That can be the case if the operating system closes all other notifications for
				// Kaidan after closing one.
				// It makes sense for the case when the corresponding chat is opened but not for
				// other actions.
				// That behavior was observed with GNOME after closing one notification when Kaidan
				// was not hidden.
				// There will never be more than 3 open notifications because otherwise it could
				// result in an indefinite loop of closing and opening notifications.
				//
				// Using 'notification->ref()' together with 'notification->sendEvent()' does not
				// work.
				// Thus, 'sendMessageNotification()' is called again.
				if (isPersistentNotification && itr->isResendingEnabled && m_openNotifications.size() <= 3) {
					sendMessageNotification(accountJid, chatJid, messageId, timestamp, messageBody);
				}

				m_openNotifications.erase(itr);
				break;
			}
		}
	});

	notification->sendEvent();
}

void Notifications::closeMessageNotifications(const QString &accountJid, const QString &chatJid, const QDateTime &timestamp, const QUuid &excludedNotificationId)
{
	for (auto &notificationWrapper : m_openNotifications) {
		if (notificationWrapper.id != excludedNotificationId &&
			notificationWrapper.accountJid == accountJid &&
			notificationWrapper.chatJid == chatJid &&
			notificationWrapper.timestamp <= timestamp) {
			notificationWrapper.isResendingEnabled = false;
			notificationWrapper.notification->close();
		}
	}
}

void Notifications::disableResending(bool isPersistentNotification, const QUuid &notificationId)
{
	if (isPersistentNotification) {
		for (auto &notificationWrapper : m_openNotifications) {
			if (notificationWrapper.id == notificationId) {
				notificationWrapper.isResendingEnabled = false;
				break;
			}
		}
	}

}
#else
void Notifications::sendMessageNotification(const QString&, const QString&, const QString&, const QDateTime&, const QString&)
{
}

void Notifications::closeMessageNotifications(const QString &accountJid, const QString &chatJid, const QDateTime &timestamp, const QUuid &excludedNotificationId)
{
}

void Notifications::disableResending(bool isPersistentNotification, const QUuid &notificationId)
{
}
#endif // HAVE_KNOTIFICATIONS
