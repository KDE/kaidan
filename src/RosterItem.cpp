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

std::strong_ordering RosterItem::operator<=>(const RosterItem &other) const
{
    // If both items are un-/pinned and one of them has a draft message, the one with the draft message is above the other one.
    if (((pinningPosition != -1) == (other.pinningPosition != -1))
        && ((lastMessageDeliveryState == Enums::DeliveryState::Draft) != (other.lastMessageDeliveryState == Enums::DeliveryState::Draft))) {
        return lastMessageDeliveryState == Enums::DeliveryState::Draft ? std::strong_ordering::less : std::strong_ordering::greater;
    }

    // If either item is pinned (pinningPosition != -1), compare based on that.
    // An item with a higher pinningPosition is above an item with a lower one.
    if (pinningPosition != -1 || other.pinningPosition != -1) {
        return other.pinningPosition <=> pinningPosition;
    }

    // If neither item is pinned, compare based on the last message date/time.
    // An item with a more recent last message date/time is above an item with an older one.
    if (lastMessageDateTime != other.lastMessageDateTime) {
        return (lastMessageDateTime < other.lastMessageDateTime) ? std::strong_ordering::greater : std::strong_ordering::less;
    }

    if (displayName() != other.displayName()) {
        return (displayName() < other.displayName()) ? std::strong_ordering::less : std::strong_ordering::greater;
    }

    if (accountJid != other.accountJid) {
        return (accountJid < other.accountJid) ? std::strong_ordering::less : std::strong_ordering::greater;
    }

    return std::strong_ordering::equal;
}

#include "moc_RosterItem.cpp"
