// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QDateTime>
#include <QXmppRosterIq.h>

#include "Encryption.h"

/**
 * Item containing one contact / conversation.
 */
struct RosterItem
{
	Q_GADGET
public:
	Q_PROPERTY(QString accountJid MEMBER jid)
	Q_PROPERTY(QString jid MEMBER jid)
	Q_PROPERTY(QString name MEMBER name)
	Q_PROPERTY(QString displayName READ displayName CONSTANT)
	Q_PROPERTY(bool sendingPresence READ isSendingPresence CONSTANT)
	Q_PROPERTY(bool receivingPresence READ isReceivingPresence CONSTANT)
	Q_PROPERTY(QVector<QString> groups MEMBER groups)
	Q_PROPERTY(int unreadMessageCount MEMBER unreadMessages)
	Q_PROPERTY(bool chatStateSendingEnabled MEMBER chatStateSendingEnabled)
	Q_PROPERTY(bool readMarkerSendingEnabled MEMBER readMarkerSendingEnabled)
	Q_PROPERTY(bool notificationsMuted MEMBER notificationsMuted)

	RosterItem() = default;
	RosterItem(const QString &accountJid, const QXmppRosterIq::Item &item, const QDateTime &lastMessageDateTime = QDateTime::currentDateTimeUtc());

	QString displayName() const;

	bool isSendingPresence() const;
	bool isReceivingPresence() const;

	bool operator==(const RosterItem &other) const = default;
	bool operator!=(const RosterItem &other) const = default;

	bool operator<(const RosterItem &other) const;
	bool operator>(const RosterItem &other) const;
	bool operator<=(const RosterItem &other) const;
	bool operator>=(const RosterItem &other) const;

	// JID of the account that has this item.
	QString accountJid;

	// JID of the contact.
	QString jid;

	// Name of the contact.
	QString name;

	// Type of this roster item's presence subscription.
	QXmppRosterIq::Item::SubscriptionType subscription = QXmppRosterIq::Item::NotSet;

	// Roster groups (i.e., labels) used for filtering (e.g., "Family", "Friends" etc.).
	QVector<QString> groups;

	// End-to-end encryption used for this roster item.
	Encryption::Enum encryption = Encryption::Omemo2;

	// Number of messages unread by the user.
	int unreadMessages = 0;

	// Last activity of the conversation, e.g., when the last message was exchanged or a draft
	// stored.
	// This is used to display the date and to sort the contacts on the roster page´.
	QDateTime lastMessageDateTime = QDateTime::currentDateTimeUtc();

	// Last message of the conversation.
	QString lastMessage;

	// Last message i.e read by the receiver.
	QString lastReadOwnMessageId;

	// Last message i.e read by the user.
	QString lastReadContactMessageId;
	
	// Draft message i.e written by the user but not yet sent.
	QString draftMessageId;

	// Whether a read marker for lastReadContactMessageId is waiting to be sent.
	bool readMarkerPending = false;

	// Position within the pinned items.
	// The higher the number, the higher the item is at the top of the pinned items.
	// The first pinned item has the position 0.
	// -1 is used for unpinned items.
	int pinningPosition = -1;

	// Whether chat states are sent to this roster item.
	bool chatStateSendingEnabled = true;

	// Whether read markers are sent to this roster item.
	bool readMarkerSendingEnabled = true;

	// Whether notifications are muted.
	bool notificationsMuted = false;
};

Q_DECLARE_METATYPE(RosterItem)
