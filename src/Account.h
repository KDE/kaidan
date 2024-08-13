// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QObject>
#include <QString>

class Account
{
	Q_GADGET

	Q_PROPERTY(ContactNotificationRule contactNotificationRule MEMBER contactNotificationRule)
	Q_PROPERTY(GroupChatNotificationRule groupChatNotificationRule MEMBER groupChatNotificationRule)

public:
	/**
	 * Default rule to inform the user about incoming messages from contacts.
	 */
	enum class ContactNotificationRule {
		Never,          ///< Never notify.
		PresenceOnly,   ///< Notify only for contacts receiving presence.
		Always,         ///< Always notify.
		Default = Always,
	};
	Q_ENUM(ContactNotificationRule)

	/**
	 * Default rule to inform the user about incoming messages from group chats.
	 */
	enum class GroupChatNotificationRule {
		Never,      ///< Never notify.
		Mentioned,  ///< Notify only if the user is mentioned.
		Always,     ///< Always notify.
		Default = Always,
	};
	Q_ENUM(GroupChatNotificationRule)

	/**
	 * Default rule to automatically download media for all roster items of an account.
	 */
	enum class AutomaticMediaDownloadsRule {
		Never,        ///< Never automatically download files.
		PresenceOnly, ///< Automatically download files only for contacts receiving presence.
		Always,       ///< Always automatically download files.
		Default = PresenceOnly,
	};
	Q_ENUM(AutomaticMediaDownloadsRule)

	Account() = default;

	bool operator==(const Account &other) const = default;
	bool operator!=(const Account &other) const = default;

	bool operator<(const Account &other) const;
	bool operator>(const Account &other) const;
	bool operator<=(const Account &other) const;
	bool operator>=(const Account &other) const;

	QString jid;
	QString name;
	QString latestMessageStanzaId;
	QDateTime latestMessageStanzaTimestamp;
	qint64 httpUploadLimit = 0;
	ContactNotificationRule contactNotificationRule = ContactNotificationRule::Default;
	GroupChatNotificationRule groupChatNotificationRule = GroupChatNotificationRule::Default;
};

Q_DECLARE_METATYPE(Account)
Q_DECLARE_METATYPE(Account::ContactNotificationRule)
Q_DECLARE_METATYPE(Account::GroupChatNotificationRule)
Q_DECLARE_METATYPE(Account::AutomaticMediaDownloadsRule)
