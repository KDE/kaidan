// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterItem.h"

// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "AccountController.h"

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
    groups = {rosterGroups.cbegin(), rosterGroups.cend()};
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

RosterItem::EffectiveNotificationRule RosterItem::effectiveNotificationRule() const
{
    switch (notificationRule) {
    case NotificationRule::Account: {
        const auto *accountSettings = AccountController::instance()->account(accountJid)->settings();

        if (isGroupChat()) {
            switch (accountSettings->groupChatNotificationRule()) {
            case AccountSettings::GroupChatNotificationRule::Never:
                return EffectiveNotificationRule::Never;
            case AccountSettings::GroupChatNotificationRule::Mentioned:
                return EffectiveNotificationRule::Mentioned;
            case AccountSettings::GroupChatNotificationRule::Always:
                return EffectiveNotificationRule::Always;
            }
        }

        switch (accountSettings->contactNotificationRule()) {
        case AccountSettings::ContactNotificationRule::Never:
            return EffectiveNotificationRule::Never;
        case AccountSettings::ContactNotificationRule::PresenceOnly:
            if (isReceivingPresence()) {
                return EffectiveNotificationRule::Always;
            }

            return EffectiveNotificationRule::Never;
        case AccountSettings::ContactNotificationRule::Always:
            return EffectiveNotificationRule::Always;
        }

        Q_UNREACHABLE();
    }
    case NotificationRule::Never:
        return EffectiveNotificationRule::Never;
    case NotificationRule::Mentioned:
        return EffectiveNotificationRule::Mentioned;
    case NotificationRule::Always:
        return EffectiveNotificationRule::Always;
    }

    Q_UNREACHABLE();
}

bool RosterItem::operator<(const RosterItem &other) const
{
    if (pinningPosition == -1 && other.pinningPosition == -1) {
        if (lastMessageDateTime != other.lastMessageDateTime) {
            return lastMessageDateTime > other.lastMessageDateTime;
        }

        if (displayName() != other.displayName()) {
            return displayName() < other.displayName();
        }

        return accountJid < other.accountJid;
    }

    return pinningPosition > other.pinningPosition;
}

bool RosterItem::operator>(const RosterItem &other) const
{
    if (pinningPosition == -1 && other.pinningPosition == -1) {
        if (lastMessageDateTime != other.lastMessageDateTime) {
            return lastMessageDateTime < other.lastMessageDateTime;
        }

        if (displayName() != other.displayName()) {
            return displayName() > other.displayName();
        }

        return accountJid > other.accountJid;
    }

    return pinningPosition < other.pinningPosition;
}

bool RosterItem::operator<=(const RosterItem &other) const
{
    if (pinningPosition == -1 && other.pinningPosition == -1) {
        if (lastMessageDateTime != other.lastMessageDateTime) {
            return lastMessageDateTime >= other.lastMessageDateTime;
        }

        if (displayName() != other.displayName()) {
            return displayName() <= other.displayName();
        }

        return accountJid <= other.accountJid;
    }

    return pinningPosition >= other.pinningPosition;
}

bool RosterItem::operator>=(const RosterItem &other) const
{
    if (pinningPosition == -1 && other.pinningPosition == -1) {
        if (lastMessageDateTime != other.lastMessageDateTime) {
            return lastMessageDateTime <= other.lastMessageDateTime;
        }

        if (displayName() != other.displayName()) {
            return displayName() >= other.displayName();
        }

        return accountJid >= other.accountJid;
    }

    return pinningPosition <= other.pinningPosition;
}

#include "moc_RosterItem.cpp"
