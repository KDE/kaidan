// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterItem.h"

#include <QXmppUtils.h>

RosterItem::RosterItem(const QString &accountJid, const QXmppRosterIq::Item &item)
    : accountJid(accountJid)
    , jid(item.bareJid())
    , subscription(item.subscriptionType())
    , groupChatParticipantId(item.mixParticipantId())
{
    if (!item.isMixChannel()) {
        name = item.name();
    }

    const auto rosterGroups = item.groups();
    groups = QList(rosterGroups.cbegin(), rosterGroups.cend());
}

QString RosterItem::displayName() const
{
    if (name.isEmpty()) {
        // Return the JID if the roster item is created by a RosterItemWatcher while not being an
        // actual item in the roster.
        if (accountJid.isEmpty()) {
            return jid;
        }

        // Consider the roster item as a chat with oneself if its JID matches the own JID.
        if (jid == accountJid) {
            return QObject::tr("Notes");
        }

        if (!groupChatName.isEmpty()) {
            return groupChatName;
        }

        const auto username = QXmppUtils::jidToUser(jid);

        // Return the domain in case of a server as a roster item (for service announcements).
        if (username.isEmpty()) {
            return jid;
        }

        return username;
    }

    return name;
}

bool RosterItem::isSendingPresence() const
{
    return subscription == QXmppRosterIq::Item::To || subscription == QXmppRosterIq::Item::Both;
}

bool RosterItem::isReceivingPresence() const
{
    return subscription == QXmppRosterIq::Item::From || subscription == QXmppRosterIq::Item::Both;
}

bool RosterItem::isGroupChat() const
{
    return !groupChatParticipantId.isEmpty();
}

bool RosterItem::isPublicGroupChat() const
{
    return groupChatFlags.testFlag(GroupChatFlag::Public);
}

bool RosterItem::isDeletedGroupChat() const
{
    return groupChatFlags.testFlag(GroupChatFlag::Deleted);
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

#include "moc_RosterItem.cpp"
