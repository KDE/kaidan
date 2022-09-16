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
#include <QXmppRosterIq.h>

#include "Encryption.h"

/**
 * Item containing one contact / conversation.
 */
struct RosterItem
{
	Q_GADGET
public:
	Q_PROPERTY(QString jid MEMBER jid)
	Q_PROPERTY(QString name MEMBER name)
	Q_PROPERTY(QString displayName READ displayName CONSTANT)
	Q_PROPERTY(int unreadMessageCount MEMBER unreadMessages)

	RosterItem() = default;
	RosterItem(const QXmppRosterIq::Item &item, const QDateTime &dateTime = QDateTime::currentDateTimeUtc());

	QString displayName() const;

	bool operator==(const RosterItem &other) const;
	bool operator!=(const RosterItem &other) const;

	bool operator<(const RosterItem &other) const;
	bool operator>(const RosterItem &other) const;
	bool operator<=(const RosterItem &other) const;
	bool operator>=(const RosterItem &other) const;

	// JID of the contact.
	QString jid;

	// Name of the contact.
	QString name;

	// Type of this roster item's presence subscription.
	QXmppRosterIq::Item::SubscriptionType subscription;

	// End-to-end encryption used for this roster item.
	Encryption::Enum encryption = Encryption::Omemo2;

	// Number of messages unread by the user.
	int unreadMessages = 0;

	// Last activity of the conversation, e.g. an incoming message.
	// This is used to sort the contacts on the roster page.
	QDateTime lastExchanged = QDateTime::currentDateTimeUtc();

	// Last message of the conversation.
	QString lastMessage;

	// Last message i.e read by the receiver.
	QString lastReadOwnMessageId;

	// Last message i.e read by the user.
	QString lastReadContactMessageId;
};

Q_DECLARE_METATYPE(RosterItem)
