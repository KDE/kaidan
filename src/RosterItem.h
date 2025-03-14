// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QDateTime>
#include <QXmppRosterIq.h>
// Kaidan
#include "Encryption.h"
#include "Enums.h"

/**
 * Item containing one contact / conversation.
 */
struct RosterItem {
    Q_GADGET

    Q_PROPERTY(QString accountJid MEMBER jid)
    Q_PROPERTY(QString jid MEMBER jid)
    Q_PROPERTY(QString name MEMBER name)
    Q_PROPERTY(QString displayName READ displayName CONSTANT)
    Q_PROPERTY(bool sendingPresence READ isSendingPresence CONSTANT)
    Q_PROPERTY(bool receivingPresence READ isReceivingPresence CONSTANT)
    Q_PROPERTY(bool isGroupChat READ isGroupChat CONSTANT)
    Q_PROPERTY(bool isPublicGroupChat READ isPublicGroupChat CONSTANT)
    Q_PROPERTY(bool isDeletedGroupChat READ isDeletedGroupChat CONSTANT)
    Q_PROPERTY(QList<QString> groups MEMBER groups)
    Q_PROPERTY(int unreadMessageCount MEMBER unreadMessages)
    Q_PROPERTY(bool chatStateSendingEnabled MEMBER chatStateSendingEnabled)
    Q_PROPERTY(bool readMarkerSendingEnabled MEMBER readMarkerSendingEnabled)
    Q_PROPERTY(RosterItem::NotificationRule notificationRule MEMBER notificationRule)
    Q_PROPERTY(RosterItem::AutomaticMediaDownloadsRule automaticMediaDownloadsRule MEMBER automaticMediaDownloadsRule)

public:
    /**
     * Rule to inform the user about incoming messages.
     */
    enum class NotificationRule {
        Account, ///< Use the account rule.
        Never, ///< Never notify.
        PresenceOnly, ///< Notify only for contacts receiving presence.
        Mentioned, ///< Notify only if the user is mentioned in a group chat.
        Always, ///< Always notify.
        Default = Account,
    };
    Q_ENUM(NotificationRule)

    /**
     * Rule to automatically download media for a roster item.
     */
    enum class AutomaticMediaDownloadsRule {
        Account, ///< Use the account rule.
        Never, ///< Never automatically download files.
        Always, ///< Always automatically download files.
        Default = Account,
    };
    Q_ENUM(AutomaticMediaDownloadsRule)

    /**
     * Flag of a group chat.
     */
    enum class GroupChatFlag {
        Public = 1, ///< The roster item is a public group chat.
        Deleted = 2 ///< The roster item is a deleted group chat.
    };
    Q_ENUM(GroupChatFlag)
    Q_DECLARE_FLAGS(GroupChatFlags, GroupChatFlag)

    RosterItem() = default;
    RosterItem(const QString &accountJid, const QXmppRosterIq::Item &item);

    QString displayName() const;

    bool isSendingPresence() const;
    bool isReceivingPresence() const;

    bool isGroupChat() const;
    bool isPublicGroupChat() const;
    bool isDeletedGroupChat() const;

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
    QList<QString> groups;

    // ID in this group chat.
    QString groupChatParticipantId;

    // Name of this group chat.
    QString groupChatName;

    // Description of this group chat.
    QString groupChatDescription;

    // Flags of this group chat.
    GroupChatFlags groupChatFlags;

    // End-to-end encryption used for this roster item.
    Encryption::Enum encryption = Encryption::Omemo2;

    // Number of messages unread by the user.
    int unreadMessages = 0;

    // Last activity of the conversation, e.g., when the last message was exchanged or a draft
    // stored.
    // This is used to display the date and to sort the contacts on the roster page´.
    QDateTime lastMessageDateTime;

    // Last message of the conversation.
    QString lastMessage;

    // Delivery state of the last message.
    Enums::DeliveryState lastMessageDeliveryState;

    // Whether the last message is an own message.
    bool lastMessageIsOwn = false;

    // Name of the last message's sender.
    QString lastMessageGroupChatSenderName;

    // ID of the last message read by the contact.
    QString lastReadOwnMessageId;

    // ID of the last message read by the user.
    QString lastReadContactMessageId;

    // Stanza ID of the latest message stanza in a group chat.
    // It can also be of a message stanza that is not displayed as a regular message (e.g., a read
    // marker).
    QString latestGroupChatMessageStanzaId;

    // Timestamp of the latest message stanza in a group chat.
    // It can also be of a message stanza that is not displayed as a regular message (e.g., a read
    // marker).
    QDateTime latestGroupChatMessageStanzaTimestamp;

    // Whether a read marker for lastReadContactMessageId is waiting to be sent.
    bool readMarkerPending = false;

    // Position within the pinned items.
    // The higher the number, the higher the item is at the top of the pinned items.
    // The first pinned item has the position 0.
    // -1 is used for unpinned items.
    int pinningPosition = -1;

    // Whether the item is selected.
    bool selected = false;

    // Whether chat states are sent to this roster item.
    bool chatStateSendingEnabled = true;

    // Whether read markers are sent to this roster item.
    bool readMarkerSendingEnabled = true;

    // Whether the user is informed about incoming messages.
    NotificationRule notificationRule = NotificationRule::Default;

    // Whether files are downloaded automatically.
    AutomaticMediaDownloadsRule automaticMediaDownloadsRule = RosterItem::AutomaticMediaDownloadsRule::Default;
};

Q_DECLARE_METATYPE(RosterItem)
Q_DECLARE_METATYPE(RosterItem::NotificationRule)
Q_DECLARE_METATYPE(RosterItem::AutomaticMediaDownloadsRule)
Q_DECLARE_METATYPE(RosterItem::GroupChatFlags)
