// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterItem.h"

#include <QXmppUtils.h>

RosterItem::RosterItem(const QXmppRosterIq::Item &item, const QDateTime &lastMessageDateTime)
	: jid(item.bareJid()), name(item.name()), subscription(item.subscriptionType()), lastMessageDateTime(lastMessageDateTime)
{
}

QString RosterItem::displayName() const
{
	return name.isEmpty() ? QXmppUtils::jidToUser(jid) : name;
}

bool RosterItem::isSendingPresence() const
{
	return subscription == QXmppRosterIq::Item::To || subscription == QXmppRosterIq::Item::Both;
}

bool RosterItem::isReceivingPresence() const
{
	return subscription == QXmppRosterIq::Item::From || subscription == QXmppRosterIq::Item::Both;
}

bool RosterItem::operator<(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastMessageDateTime != other.lastMessageDateTime) {
			return lastMessageDateTime > other.lastMessageDateTime;
		}
		return displayName().toUpper() < other.displayName().toUpper();
	}
	return pinningPosition > other.pinningPosition;
}

bool RosterItem::operator>(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastMessageDateTime != other.lastMessageDateTime) {
			return lastMessageDateTime < other.lastMessageDateTime;
		}
		return displayName().toUpper() > other.displayName().toUpper();
	}
	return pinningPosition < other.pinningPosition;
}

bool RosterItem::operator<=(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastMessageDateTime != other.lastMessageDateTime) {
			return lastMessageDateTime >= other.lastMessageDateTime;
		}
		return displayName().toUpper() <= other.displayName().toUpper();
	}
	return pinningPosition >= other.pinningPosition;
}

bool RosterItem::operator>=(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastMessageDateTime != other.lastMessageDateTime) {
			return lastMessageDateTime <= other.lastMessageDateTime;
		}
		return displayName().toUpper() >= other.displayName().toUpper();
	}
	return pinningPosition <= other.pinningPosition;
}
