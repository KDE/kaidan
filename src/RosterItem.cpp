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

#include "RosterItem.h"
#include <QXmppUtils.h>

RosterItem::RosterItem(const QXmppRosterIq::Item &item, const QDateTime &dateTime)
	: jid(item.bareJid()), name(item.name()), subscription(item.subscriptionType()), lastExchanged(dateTime)
{
}

QString RosterItem::displayName() const
{
	return name.isEmpty() ? QXmppUtils::jidToUser(jid) : name;
}

bool RosterItem::operator==(const RosterItem &other) const = default;
bool RosterItem::operator!=(const RosterItem &other) const = default;

bool RosterItem::operator<(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastExchanged != other.lastExchanged) {
			return lastExchanged > other.lastExchanged;
		}
		return displayName().toUpper() < other.displayName().toUpper();
	}
	return pinningPosition > other.pinningPosition;
}

bool RosterItem::operator>(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastExchanged != other.lastExchanged) {
			return lastExchanged < other.lastExchanged;
		}
		return displayName().toUpper() > other.displayName().toUpper();
	}
	return pinningPosition < other.pinningPosition;
}

bool RosterItem::operator<=(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastExchanged != other.lastExchanged) {
			return lastExchanged >= other.lastExchanged;
		}
		return displayName().toUpper() <= other.displayName().toUpper();
	}
	return pinningPosition >= other.pinningPosition;
}

bool RosterItem::operator>=(const RosterItem &other) const
{
	if (pinningPosition == -1 && other.pinningPosition == -1) {
		if (lastExchanged != other.lastExchanged) {
			return lastExchanged <= other.lastExchanged;
		}
		return displayName().toUpper() >= other.displayName().toUpper();
	}
	return pinningPosition <= other.pinningPosition;
}
