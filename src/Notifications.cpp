// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Notifications.h"

// KNotifications
#ifdef HAVE_KNOTIFICATIONS
#include <KNotification>
#endif

// Kaidan
#include "FutureUtils.h"
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

// Event IDs corresponding to the section entries in the "kaidan.notifyrc" configuration file
constexpr QStringView NEW_MESSAGE_EVENT_ID = u"new-message";
constexpr QStringView NEW_SUBSEQUENT_MESSAGE_EVENT_ID = u"new-subsequent-message";
constexpr QStringView PRESENCE_SUBSCRIPTION_REQUEST_EVENT_ID = u"presence-subscription-request";

constexpr auto SUBSEQUENT_MESSAGE_INTERVAL = 5s;
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

	auto notificationWrapperItr = std::find_if(m_openMessageNotifications.begin(), m_openMessageNotifications.end(), [&accountJid, &chatJid](const auto &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	// Update an existing notification or create a new one.
	if (notificationWrapperItr != m_openMessageNotifications.end()) {
		auto &messages = notificationWrapperItr->messages;
		messages.append(messageBody);

		// Initialize variables by known values.
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

		// Do not disturb the user when messages are received in quick succession by only playing a
		// notification sound for the first message.
		const auto initalTimestamp = notificationWrapperItr->initalTimestamp.toSecsSinceEpoch() * 1s;
		const auto currentTimestamp = QDateTime::currentSecsSinceEpoch() * 1s;
		if (currentTimestamp - initalTimestamp > SUBSEQUENT_MESSAGE_INTERVAL) {
			notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
		} else {
			notification = new KNotification(NEW_SUBSEQUENT_MESSAGE_EVENT_ID.toString());
		}

		notification->setText(notificationTextLines.join(u'\n'));

		notificationWrapperItr->isDeletionEnabled = false;
		notificationWrapperItr->notification->close();
		notificationWrapperItr->notification = notification;
	} else {
		notification = new KNotification(NEW_MESSAGE_EVENT_ID.toString());
		notification->setText(messageBody);

		MessageNotificationWrapper notificationWrapper {
			.accountJid = accountJid,
			.chatJid = chatJid,
			.initalTimestamp = QDateTime::currentDateTimeUtc(),
			.latestMessageId = messageId,
			.messages = { messageBody },
			.notification = notification
		};
		m_openMessageNotifications.append(notificationWrapper);
	}

	notification->setTitle(determineChatName(chatJid));

#ifdef DESKTOP_LINUX_ALIKE_OS
	if (IS_USING_GNOME) {
		notification->setFlags(KNotification::Persistent);
	}
#endif
#ifdef Q_OS_ANDROID
	notification->setIconName("kaidan-bw");
#endif
	notification->setDefaultAction(tr("Open"));
	notification->setActions({ tr("Mark as read") });

	connect(notification, &KNotification::defaultActivated, this, [=] {
		Q_EMIT Kaidan::instance()->openChatPageRequested(accountJid, chatJid);
		Q_EMIT Kaidan::instance()->raiseWindowRequested();
	});
	connect(notification, &KNotification::action1Activated, this, [=] {
		Q_EMIT RosterModel::instance()->updateItemRequested(chatJid, [=](RosterItem &item) {
			item.lastReadContactMessageId = messageId;
			item.unreadMessages = 0;
		});

		if (const auto item = RosterModel::instance()->findItem(chatJid); item && item->readMarkerSendingEnabled) {
			runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid, messageId]() {
				Kaidan::instance()->client()->messageHandler()->sendReadMarker(chatJid, messageId);
			});
		}
	});

	connect(notification, &KNotification::closed, this, [=, this]() {
		auto notificationWrapperItr = std::find_if(m_openMessageNotifications.begin(), m_openMessageNotifications.end(), [accountJid, chatJid](const MessageNotificationWrapper &notificationWrapper) {
			return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
		});

		if (notificationWrapperItr != m_openMessageNotifications.end()) {
			if (notificationWrapperItr->isDeletionEnabled) {
				m_openMessageNotifications.erase(notificationWrapperItr);
			} else {
				notificationWrapperItr->isDeletionEnabled = true;
			}
		}
	});

	notification->sendEvent();
}

void Notifications::closeMessageNotification(const QString &accountJid, const QString &chatJid)
{
	const auto notificationWrapperItr = std::find_if(m_openMessageNotifications.cbegin(), m_openMessageNotifications.cend(), [accountJid, chatJid](const MessageNotificationWrapper &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	if (notificationWrapperItr != m_openMessageNotifications.cend()) {
		notificationWrapperItr->notification->close();
	}
}

void Notifications::sendPresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
#ifdef DESKTOP_LINUX_ALIKE_OS
	static bool IS_USING_GNOME = qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains("GNOME", Qt::CaseInsensitive);
#endif

	auto notificationWrapperItr = std::find_if(m_openMessageNotifications.begin(), m_openMessageNotifications.end(), [&accountJid, &chatJid](const auto &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	// Only create a new notification if none exists.
	if (notificationWrapperItr != m_openMessageNotifications.end()) {
		return;
	}

	KNotification *notification = new KNotification(PRESENCE_SUBSCRIPTION_REQUEST_EVENT_ID.toString());
	notification->setTitle(determineChatName(chatJid));
	notification->setText(tr("Requests to receive your personal data"));

	PresenceSubscriptionRequestNotificationWrapper notificationWrapper {
		.accountJid = accountJid,
		.chatJid = chatJid,
		.notification = notification
	};
	m_openPresenceSubscriptionRequestNotifications.append(notificationWrapper);

#ifdef DESKTOP_LINUX_ALIKE_OS
	if (IS_USING_GNOME) {
		notification->setFlags(KNotification::Persistent);
	}
#endif
#ifdef Q_OS_ANDROID
	notification->setIconName("kaidan-bw");
#endif

	notification->setDefaultAction(tr("Open"));

	connect(notification, &KNotification::defaultActivated, this, [=] {
		Q_EMIT Kaidan::instance()->openChatPageRequested(accountJid, chatJid);
		Q_EMIT Kaidan::instance()->raiseWindowRequested();
		notification->close();
	});

	connect(notification, &KNotification::closed, this, [=, this]() {
		auto notificationWrapperItr = std::find_if(m_openPresenceSubscriptionRequestNotifications.begin(), m_openPresenceSubscriptionRequestNotifications.end(), [accountJid, chatJid](const PresenceSubscriptionRequestNotificationWrapper &notificationWrapper) {
			return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
		});

		if (notificationWrapperItr != m_openPresenceSubscriptionRequestNotifications.end()) {
			m_openPresenceSubscriptionRequestNotifications.erase(notificationWrapperItr);
		}
	});

	notification->sendEvent();
}

void Notifications::closePresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
	const auto notificationWrapperItr = std::find_if(m_openPresenceSubscriptionRequestNotifications.cbegin(), m_openPresenceSubscriptionRequestNotifications.cend(), [accountJid, chatJid](const PresenceSubscriptionRequestNotificationWrapper &notificationWrapper) {
		return notificationWrapper.accountJid == accountJid && notificationWrapper.chatJid == chatJid;
	});

	if (notificationWrapperItr != m_openPresenceSubscriptionRequestNotifications.cend()) {
		notificationWrapperItr->notification->close();
	}
}

QString Notifications::determineChatName(const QString &chatJid) const
{
	auto rosterItem = RosterModel::instance()->findItem(chatJid);
	return rosterItem ? rosterItem->displayName() : chatJid;
}
#else
void Notifications::sendMessageNotification(const QString &, const QString &, const QString &, const QString &)
{
}

void Notifications::closeMessageNotification(const QString &, const QString &)
{
}

void Notifications::sendPresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
}

void Notifications::closePresenceSubscriptionRequestNotification(const QString &accountJid, const QString &chatJid)
{
}
#endif // HAVE_KNOTIFICATIONS
