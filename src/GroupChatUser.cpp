// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUser.h"

// Kaidan
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
    // Users can have only an ID (participant in anonymous group chat), only a JID (allowed but not
    // joined or banned) or both (participant in normal group chat).
    return accountJid == other.accountJid && chatJid == other.chatJid && (id == other.id || jid == other.jid) && name == other.name && status == other.status;
}

bool GroupChatUser::operator!=(const GroupChatUser &other) const
{
    return !operator==(other);
}

bool GroupChatUser::operator<(const GroupChatUser &other) const
{
    if (status == other.status) {
        return displayName() < other.displayName();
    }

    return status < other.status;
}

#include "moc_GroupChatUser.cpp"
