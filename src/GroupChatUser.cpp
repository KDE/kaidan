// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUser.h"

#include "RosterModel.h"

QString GroupChatUser::displayName() const
{
	if (const auto rosterItem = RosterModel::instance()->findItem(jid)) {
		return rosterItem->name;
	}

	if (name.isEmpty()) {
		if (jid.isEmpty()) {
			return id;
		}

		return jid;
	}

	return name;
}

bool GroupChatUser::operator==(const GroupChatUser &other) const
{
	return other.accountJid == accountJid && other.name == name && other.jid == jid;
}

bool GroupChatUser::operator!=(const GroupChatUser &other) const
{
	return !operator==(other);
}
