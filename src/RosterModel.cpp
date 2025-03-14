// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterModel.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "AccountManager.h"
#include "ChatController.h"
#include "FutureUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterItemWatcher.h"
#include "RosterManager.h"
#include "kaidan_core_debug.h"

RosterModel *RosterModel::s_instance = nullptr;

RosterModel *RosterModel::instance()
{
    return s_instance;
}

RosterModel::RosterModel(QObject *parent)
    : QAbstractListModel(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(RosterDb::instance(), &RosterDb::itemAdded, this, &RosterModel::addItem);
    connect(RosterDb::instance(), &RosterDb::itemUpdated, this, &RosterModel::updateItem);
    connect(RosterDb::instance(), &RosterDb::itemRemoved, this, &RosterModel::removeItem);
    connect(RosterDb::instance(), &RosterDb::itemsRemoved, this, &RosterModel::removeItems);

    connect(MessageDb::instance(), &MessageDb::messageAdded, this, &RosterModel::handleMessageAdded);
    connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &RosterModel::handleMessageUpdated);
    connect(MessageDb::instance(), &MessageDb::draftMessageAdded, this, &RosterModel::handleDraftMessageAdded);
    connect(MessageDb::instance(), &MessageDb::draftMessageUpdated, this, &RosterModel::handleDraftMessageUpdated);
    connect(MessageDb::instance(), &MessageDb::draftMessageRemoved, this, &RosterModel::handleDraftMessageRemoved);
    connect(MessageDb::instance(), &MessageDb::messageRemoved, this, &RosterModel::handleMessageRemoved);

    connect(AccountManager::instance(), &AccountManager::accountChanged, this, [this] {
        beginResetModel();
        m_items.clear();
        endResetModel();

        await(RosterDb::instance()->fetchItems(), this, [this](const QList<RosterItem> &items) {
            handleItemsFetched(items);
        });
    });

    connect(AccountManager::instance(), &AccountManager::accountChanged, this, &RosterModel::handleAccountChanged);
}

RosterModel::~RosterModel()
{
    s_instance = nullptr;
}

int RosterModel::rowCount(const QModelIndex &) const
{
    return m_items.size();
}

QHash<int, QByteArray> RosterModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[AccountJidRole] = QByteArrayLiteral("accountJid");
    roles[JidRole] = QByteArrayLiteral("jid");
    roles[NameRole] = QByteArrayLiteral("name");
    roles[GroupsRole] = QByteArrayLiteral("groups");
    roles[IsGroupChatRole] = QByteArrayLiteral("isGroupChat");
    roles[IsPublicGroupChatRole] = QByteArrayLiteral("isPublicGroupChat");
    roles[IsDeletedGroupChatRole] = QByteArrayLiteral("isDeletedGroupChat");
    roles[LastMessageDateTimeRole] = QByteArrayLiteral("lastMessageDateTime");
    roles[UnreadMessagesRole] = QByteArrayLiteral("unreadMessages");
    roles[LastMessageRole] = QByteArrayLiteral("lastMessage");
    roles[LastMessageIsDraftRole] = QByteArrayLiteral("lastMessageIsDraft");
    roles[LastMessageIsOwnRole] = QByteArrayLiteral("lastMessageIsOwn");
    roles[LastMessageGroupChatSenderNameRole] = QByteArrayLiteral("lastMessageGroupChatSenderName");
    roles[PinnedRole] = QByteArrayLiteral("pinned");
    roles[SelectedRole] = QByteArrayLiteral("selected");
    roles[NotificationRuleRole] = QByteArrayLiteral("notificationRule");
    return roles;
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not get data from roster model." << index << role;
        return {};
    }

    const auto &item = m_items.at(index.row());

    switch (role) {
    case AccountJidRole:
        return item.accountJid;
    case JidRole:
        return item.jid;
    case NameRole:
        return item.displayName();
    case GroupsRole:
        return QVariant::fromValue(item.groups);
    case IsGroupChatRole:
        return item.isGroupChat();
    case IsPublicGroupChatRole:
        return item.groupChatFlags.testFlag(RosterItem::GroupChatFlag::Public);
    case IsDeletedGroupChatRole:
        return item.groupChatFlags.testFlag(RosterItem::GroupChatFlag::Deleted);
    case LastMessageDateTimeRole: {
        // "lastMessageDateTime" is used for sorting the roster items.
        // Thus, each new item has the current date as its default value.
        // But that date should only be displayed when there is a last (exchanged or draft) message.
        if (!item.lastMessage.isEmpty() || item.lastMessageDeliveryState == Enums::DeliveryState::Draft) {
            return formatLastMessageDateTime(item.lastMessageDateTime);
        }

        // The user interface needs a valid string.
        // Thus, a null string (i.e., "{}") must not be returned.
        return QString();
    }
    case UnreadMessagesRole:
        return item.unreadMessages;
    case LastMessageRole:
        return item.lastMessage;
    case LastMessageIsDraftRole:
        return item.lastMessageDeliveryState == Enums::DeliveryState::Draft;
    case LastMessageIsOwnRole:
        return item.lastMessageIsOwn;
    case LastMessageGroupChatSenderNameRole:
        return item.lastMessageGroupChatSenderName;
    case PinnedRole:
        return item.pinningPosition >= 0;
    case SelectedRole:
        return item.selected;
    case NotificationRuleRole:
        if (item.notificationRule == RosterItem::NotificationRule::Account) {
            if (item.isGroupChat()) {
                switch (AccountManager::instance()->account().groupChatNotificationRule) {
                case Account::GroupChatNotificationRule::Never:
                    return QVariant::fromValue(RosterItem::NotificationRule::Never);
                case Account::GroupChatNotificationRule::Mentioned:
                    return QVariant::fromValue(RosterItem::NotificationRule::Mentioned);
                case Account::GroupChatNotificationRule::Always:
                    return QVariant::fromValue(RosterItem::NotificationRule::Always);
                }
            }

            switch (AccountManager::instance()->account().contactNotificationRule) {
            case Account::ContactNotificationRule::Never:
                return QVariant::fromValue(RosterItem::NotificationRule::Never);
            case Account::ContactNotificationRule::PresenceOnly:
                if (item.isReceivingPresence()) {
                    return QVariant::fromValue(RosterItem::NotificationRule::Always);
                } else {
                    return QVariant::fromValue(RosterItem::NotificationRule::Never);
                }
            case Account::ContactNotificationRule::Always:
                return QVariant::fromValue(RosterItem::NotificationRule::Always);
            }
        }

        return QVariant::fromValue(item.notificationRule);
    }
    return {};
}

bool RosterModel::hasItem(const QString &jid) const
{
    return findItem(jid).has_value();
}

QStringList RosterModel::accountJids() const
{
    QStringList accountJids;

    std::for_each(m_items.cbegin(), m_items.cend(), [&accountJids](const RosterItem &item) {
        if (const auto accountJid = item.accountJid; !accountJids.contains(accountJid)) {
            accountJids.append(accountJid);
        }
    });

    std::sort(accountJids.begin(), accountJids.end());

    return accountJids;
}

QStringList RosterModel::groups() const
{
    QStringList groups;

    std::for_each(m_items.cbegin(), m_items.cend(), [&groups](const RosterItem &item) {
        std::for_each(item.groups.cbegin(), item.groups.cend(), [&](const QString &group) {
            if (!groups.contains(group)) {
                groups.append(group);
            }
        });
    });

    std::sort(groups.begin(), groups.end());

    return groups;
}

void RosterModel::updateGroup(const QString &oldGroup, const QString &newGroup)
{
    for (const auto &item : std::as_const(m_items)) {
        if (auto groups = item.groups; groups.contains(oldGroup)) {
            groups.removeOne(oldGroup);
            groups.append(newGroup);

            Q_EMIT Kaidan::instance()->client()->rosterManager()->updateGroupsRequested(item.jid, item.name, groups);
        }
    }
}

void RosterModel::removeGroup(const QString &group)
{
    for (const auto &item : std::as_const(m_items)) {
        if (auto groups = item.groups; groups.contains(group)) {
            groups.removeOne(group);

            Q_EMIT Kaidan::instance()->client()->rosterManager()->updateGroupsRequested(item.jid, item.name, groups);
        }
    }
}

bool RosterModel::isPresenceSubscribedByItem(const QString &, const QString &jid) const
{
    if (auto item = findItem(jid)) {
        return item->subscription == QXmppRosterIq::Item::From || item->subscription == QXmppRosterIq::Item::Both;
    }
    return false;
}

std::optional<Encryption::Enum> RosterModel::itemEncryption(const QString &, const QString &jid) const
{
    if (auto item = findItem(jid)) {
        return item->encryption;
    }
    return {};
}

void RosterModel::setItemEncryption(const QString &, const QString &jid, Encryption::Enum encryption)
{
    RosterDb::instance()->updateItem(jid, [encryption](RosterItem &item) {
        item.encryption = encryption;
    });
}

void RosterModel::setItemEncryption(const QString &, Encryption::Enum encryption)
{
    for (const auto &item : std::as_const(m_items)) {
        RosterDb::instance()->updateItem(item.jid, [encryption](RosterItem &item) {
            item.encryption = encryption;
        });
    }
}

RosterModel::AddContactByUriResult RosterModel::addContactByUri(const QString &accountJid, const QString &uriString)
{
    if (const auto uriParsingResult = QXmppUri::fromString(uriString); std::holds_alternative<QXmppUri>(uriParsingResult)) {
        const auto uri = std::get<QXmppUri>(uriParsingResult);
        const auto jid = uri.jid();

        if (jid.isEmpty()) {
            return AddContactByUriResult::InvalidUri;
        }

        if (RosterModel::instance()->hasItem(jid)) {
            Q_EMIT Kaidan::instance()->openChatPageRequested(accountJid, jid);
            return AddContactByUriResult::ContactExists;
        }

        Q_EMIT Kaidan::instance()->client()->rosterManager()->addContactRequested(jid);

        return AddContactByUriResult::AddingContact;
    }

    return AddContactByUriResult::InvalidUri;
}

QString RosterModel::lastReadOwnMessageId(const QString &, const QString &jid) const
{
    if (auto item = findItem(jid))
        return item->lastReadOwnMessageId;
    return {};
}

QString RosterModel::lastReadContactMessageId(const QString &, const QString &jid) const
{
    if (auto item = findItem(jid))
        return item->lastReadContactMessageId;
    return {};
}

void RosterModel::sendPendingReadMarkers(const QString &)
{
    for (const auto &item : std::as_const(m_items)) {
        if (const auto messageId = item.lastReadContactMessageId; item.readMarkerPending && !messageId.isEmpty()) {
            const auto chatJid = item.jid;

            if (item.readMarkerSendingEnabled) {
                MessageController::instance()->sendReadMarker(chatJid, messageId);
            }

            RosterDb::instance()->updateItem(chatJid, [](RosterItem &item) {
                item.readMarkerPending = false;
            });
        }
    }
}

std::optional<RosterItem> RosterModel::findItem(const QString &jid) const
{
    for (const auto &item : std::as_const(m_items)) {
        if (item.jid == jid) {
            return item;
        }
    }

    return {};
}

const QList<RosterItem> &RosterModel::items() const
{
    return m_items;
}

void RosterModel::pinItem(const QString &, const QString &jid)
{
    RosterDb::instance()->updateItem(jid, [highestPinningPosition = m_items.at(0).pinningPosition](RosterItem &item) {
        item.pinningPosition = highestPinningPosition + 1;
    });
}

void RosterModel::unpinItem(const QString &, const QString &jid)
{
    if (const auto itemBeingUnpinned = findItem(jid)) {
        for (const auto &item : std::as_const(m_items)) {
            // Decrease the pinning position of the pinned items with higher pinning positions than
            // the pinning position of the item being pinned.
            if (item.pinningPosition > itemBeingUnpinned->pinningPosition) {
                RosterDb::instance()->updateItem(item.jid, [](RosterItem &item) {
                    item.pinningPosition -= 1;
                });
            }
        }

        // Reset the pinning position of the item being unpinned.
        RosterDb::instance()->updateItem(jid, [](RosterItem &item) {
            item.pinningPosition = -1;
        });
    }
}

void RosterModel::reorderPinnedItem(const QString &, const QString &, int oldIndex, int newIndex)
{
    if (oldIndex < 0 || oldIndex >= m_items.size() || newIndex < 0 || newIndex >= m_items.size() || oldIndex == newIndex) {
        return;
    }

    const int min = std::min(oldIndex, newIndex);
    const int max = std::max(oldIndex, newIndex);
    const auto movedUpwards = newIndex < oldIndex;
    const auto itemAtNewIndex = m_items[newIndex];
    const int pinningPosition = itemAtNewIndex.pinningPosition;

    // Do not reorder anything if the item would go out of the range of the pinned items.
    // That happens when the item is dragged below unpinned items.
    if (pinningPosition < 0) {
        return;
    }

    for (int i = min; i <= max; ++i) {
        const auto &item = m_items[i];

        RosterDb::instance()->updateItem(item.jid, [movedUpwards, i, oldIndex, pinningPosition](RosterItem &item) {
            if (i == oldIndex) {
                // Set the new moved item's pinningPosition.
                item.pinningPosition = pinningPosition;
            } else {
                if (movedUpwards) {
                    // Decrement pinningPosition of the items that are now below the moved item.
                    --item.pinningPosition;
                } else {
                    // Increment pinningPosition of the items that are now above the moved item.
                    ++item.pinningPosition;
                }
            }
        });
    }
}

void RosterModel::toggleSelected(const QString &, const QString &jid)
{
    for (int i = 0; i < m_items.size(); i++) {
        if (auto &item = m_items[i]; item.jid == jid) {
            item.selected = !item.selected;
            Q_EMIT dataChanged(index(i), index(i), {SelectedRole});
            return;
        }
    }
}

void RosterModel::resetSelected()
{
    for (int i = 0; i < m_items.size(); i++) {
        if (auto &item = m_items[i]; item.selected) {
            item.selected = false;
            Q_EMIT dataChanged(index(i), index(i), {SelectedRole});
        }
    }
}

void RosterModel::setChatStateSendingEnabled(const QString &, const QString &jid, bool chatStateSendingEnabled)
{
    RosterDb::instance()->updateItem(jid, [=](RosterItem &item) {
        item.chatStateSendingEnabled = chatStateSendingEnabled;
    });
}

void RosterModel::setReadMarkerSendingEnabled(const QString &, const QString &jid, bool readMarkerSendingEnabled)
{
    RosterDb::instance()->updateItem(jid, [=](RosterItem &item) {
        item.readMarkerSendingEnabled = readMarkerSendingEnabled;
    });
}

void RosterModel::setNotificationRule(const QString &, const QString &jid, RosterItem::NotificationRule notificationRule)
{
    RosterDb::instance()->updateItem(jid, [=](RosterItem &item) {
        item.notificationRule = notificationRule;
    });
}

void RosterModel::setAutomaticMediaDownloadsRule(const QString &, const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule)
{
    RosterDb::instance()->updateItem(jid, [rule](RosterItem &item) {
        item.automaticMediaDownloadsRule = rule;
    });
}

void RosterModel::handleItemsFetched(const QList<RosterItem> &items)
{
    beginResetModel();
    m_items = items;
    std::sort(m_items.begin(), m_items.end());
    endResetModel();

    for (const auto &item : std::as_const(m_items)) {
        RosterItemNotifier::instance().notifyWatchers(item.jid, item);
    }

    Q_EMIT accountJidsChanged();
    Q_EMIT groupsChanged();
}

void RosterModel::addItem(const RosterItem &item)
{
    insertItem(positionToAdd(item), item);
}

void RosterModel::updateItem(const RosterItem &item)
{
    for (int i = 0; i < m_items.size(); i++) {
        const auto jid = item.jid;
        if (const auto oldItem = m_items.at(i); oldItem.jid == jid) {
            const auto oldGroups = groups();

            m_items.replace(i, item);

            // Apply old settings that are not stored in the database.
            auto &newItem = m_items[i];
            newItem = item;
            newItem.selected = oldItem.selected;

            Q_EMIT dataChanged(index(i), index(i));
            RosterItemNotifier::instance().notifyWatchers(jid, item);
            updateItemPosition(i);

            if (oldGroups != groups()) {
                Q_EMIT groupsChanged();
            }

            // TODO: Update "lastMessageGroupChatSenderName" if its corresponding roster item changes its name
            // TODO: Current problem: last message sender JID is not cached and check with lastMessageGroupChatSenderName may result in a conflict if a group
            // chat user has the same name as a roster item

            return;
        }
    }
}

void RosterModel::removeItem(const QString &accountJid, const QString &jid)
{
    for (int i = 0; i < m_items.size(); i++) {
        const RosterItem &item = m_items.at(i);

        if (item.accountJid == accountJid && item.jid == jid) {
            auto oldGroupCount = groups().size();

            beginRemoveRows(QModelIndex(), i, i);
            m_items.remove(i);
            endRemoveRows();

            RosterItemNotifier::instance().notifyWatchers(jid, std::nullopt);

            if (!accountJids().contains(accountJid)) {
                Q_EMIT accountJidsChanged();
            }

            if (oldGroupCount < groups().size()) {
                Q_EMIT groupsChanged();
            }

            if (accountJid == ChatController::instance()->accountJid() && jid == ChatController::instance()->chatJid()) {
                Q_EMIT Kaidan::instance()->closeChatPageRequested();
            }

            Q_EMIT itemRemoved(item.accountJid, jid);

            return;
        }
    }
}

void RosterModel::removeItems(const QString &accountJid)
{
    for (int i = 0; i < m_items.size(); i++) {
        const RosterItem &item = m_items.at(i);

        if (item.accountJid == accountJid) {
            auto oldGroupCount = groups().size();

            beginRemoveRows(QModelIndex(), i, i);
            m_items.remove(i);
            endRemoveRows();

            const auto &jid = item.jid;
            RosterItemNotifier::instance().notifyWatchers(jid, std::nullopt);

            if (!accountJids().contains(accountJid)) {
                Q_EMIT accountJidsChanged();
            }

            if (oldGroupCount < groups().size()) {
                Q_EMIT groupsChanged();
            }

            if (accountJid == ChatController::instance()->accountJid() && jid == ChatController::instance()->chatJid()) {
                Q_EMIT Kaidan::instance()->closeChatPageRequested();
            }
        }
    }
}

void RosterModel::handleAccountChanged()
{
    Q_EMIT dataChanged(index(0), index(m_items.size() - 1), {NotificationRuleRole});
}

void RosterModel::handleMessageAdded(const Message &message, MessageOrigin origin)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
        return item.jid == message.chatJid;
    });

    // roster item not found
    if (itr == m_items.end()) {
        return;
    }

    await(updateLastMessage(itr, message), this, [this, message, origin, itr](QList<int> &&changedRoles) mutable {
        // unread messages counter
        std::optional<int> newUnreadMessages;

        if (message.isOwn) {
            // if we sent a message (with another device), reset counter
            newUnreadMessages = 0;
        } else {
            // increase counter if message is new
            switch (origin) {
            case MessageOrigin::Stream:
            case MessageOrigin::UserInput:
            case MessageOrigin::MamCatchUp:
                newUnreadMessages = itr->unreadMessages + 1;
            case MessageOrigin::MamBacklog:
            case MessageOrigin::MamInitial:
                break;
            }
        }

        if (newUnreadMessages) {
            itr->unreadMessages = *newUnreadMessages;
            changedRoles << int(UnreadMessagesRole);

            RosterDb::instance()->updateItem(message.chatJid, [newCount = *newUnreadMessages](RosterItem &item) {
                item.unreadMessages = newCount;
            });
        }

        if (!changedRoles.isEmpty()) {
            updateItemPosition(informAboutChangedData(itr, changedRoles));
        }
    });
}

void RosterModel::handleMessageUpdated(const Message &message)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
        return item.jid == message.chatJid;
    });

    // Skip further processing if the contact could not be found.
    if (itr == m_items.end()) {
        return;
    }

    await(updateLastMessage(itr, message), this, [this, itr](QList<int> &&changedRoles) mutable {
        if (!changedRoles.isEmpty()) {
            informAboutChangedData(itr, changedRoles);
        }
    });
}

void RosterModel::handleDraftMessageAdded(const Message &message)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
        return item.jid == message.chatJid;
    });

    // roster item not found
    if (itr == m_items.end()) {
        return;
    }

    itr->lastMessageDateTime = message.timestamp;
    itr->lastMessageDeliveryState = Enums::DeliveryState::Draft;
    itr->lastMessage = message.previewText();

    updateOnDraftMessageChanged(itr);
}

void RosterModel::handleDraftMessageUpdated(const Message &message)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
        return item.jid == message.chatJid;
    });

    // roster item not found
    if (itr == m_items.end()) {
        return;
    }

    itr->lastMessageDateTime = message.timestamp;
    itr->lastMessage = message.previewText();

    updateOnDraftMessageChanged(itr);
}

void RosterModel::handleDraftMessageRemoved(const Message &newLastMessage)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&newLastMessage](const RosterItem &item) {
        return item.accountJid == newLastMessage.accountJid && item.jid == newLastMessage.chatJid;
    });

    // roster item not found
    if (itr == m_items.end()) {
        return;
    }

    itr->lastMessageDateTime = newLastMessage.timestamp;
    itr->lastMessageDeliveryState = newLastMessage.deliveryState;
    itr->lastMessage = newLastMessage.previewText();

    updateOnDraftMessageChanged(itr);
}

void RosterModel::handleMessageRemoved(const Message &newLastMessage)
{
    auto itr = std::find_if(m_items.begin(), m_items.end(), [&newLastMessage](const RosterItem &item) {
        return item.jid == newLastMessage.chatJid;
    });

    // Skip further processing if the contact could not be found.
    if (itr == m_items.end()) {
        return;
    }

    await(updateLastMessage(itr, newLastMessage, false), this, [this, itr](QList<int> &&changedRoles) mutable {
        if (!changedRoles.isEmpty()) {
            informAboutChangedData(itr, changedRoles);
        }
    });
}

QFuture<QList<int>> RosterModel::updateLastMessage(QList<RosterItem>::Iterator &itr, const Message &message, bool onlyUpdateIfNewerOrAtSameAge)
{
    QFutureInterface<QList<int>> interface(QFutureInterfaceBase::Started);

    // If desired, only set the new message as the current last message if it is newer than
    // the current one or at the same age.
    // That makes it possible to use the previous message as the new last message if the current
    // last message is empty.
    if (!itr->lastMessage.isEmpty() && (onlyUpdateIfNewerOrAtSameAge && itr->lastMessageDateTime > message.timestamp)) {
        reportFinishedResult(interface, {});
    }

    // The new message is only set as the current last message if they are different and there
    // is no draft message.
    if (const auto lastMessage = message.previewText();
        (itr->lastMessage != lastMessage || itr->lastMessageDateTime != message.timestamp) && itr->lastMessageDeliveryState != Enums::DeliveryState::Draft) {
        itr->lastMessageDateTime = message.timestamp;
        itr->lastMessage = lastMessage;
        itr->lastMessageIsOwn = message.isOwn;

        if (itr->isGroupChat()) {
            itr->lastMessageGroupChatSenderName = determineGroupChatSenderName(message);
        } else {
            itr->lastMessageGroupChatSenderName.clear();
        }

        static const QList<int> changedRoles = {
            int(LastMessageRole),
            int(LastMessageIsOwnRole),
            int(LastMessageGroupChatSenderNameRole),
            int(LastMessageDateTimeRole),
        };

        reportFinishedResult(interface, std::move(changedRoles));
    } else {
        reportFinishedResult(interface, {});
    }

    return interface.future();
}

void RosterModel::updateOnDraftMessageChanged(QList<RosterItem>::Iterator &itr)
{
    static const QList<int> changedRoles = {int(LastMessageDateTimeRole), int(LastMessageRole), int(LastMessageIsDraftRole)};

    updateItemPosition(informAboutChangedData(itr, changedRoles));
}

int RosterModel::informAboutChangedData(QList<RosterItem>::Iterator &itr, const QList<int> &changedRoles)
{
    const auto i = std::distance(m_items.begin(), itr);
    const auto modelIndex = index(i);
    Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);

    RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

    return i;
}

void RosterModel::insertItem(int index, const RosterItem &item)
{
    auto newAccountJidAdded = !accountJids().contains(item.accountJid);
    auto oldGroupCount = groups().size();

    beginInsertRows(QModelIndex(), index, index);
    m_items.insert(index, item);
    endInsertRows();

    const auto jid = item.jid;
    RosterItemNotifier::instance().notifyWatchers(jid, item);
    Q_EMIT itemAdded(item.accountJid, jid);

    if (newAccountJidAdded) {
        Q_EMIT accountJidsChanged();
    }

    if (oldGroupCount < groups().size()) {
        Q_EMIT groupsChanged();
    }
}

void RosterModel::updateItemPosition(int currentIndex)
{
    int newIndex = positionToMove(currentIndex);

    if (currentIndex == newIndex) {
        return;
    }

    beginMoveRows(QModelIndex(), currentIndex, currentIndex, QModelIndex(), newIndex);

    // Cover both cases:
    // 1. Moving to a higher index
    // 2. Moving to a lower index
    if (currentIndex < newIndex) {
        m_items.move(currentIndex, newIndex - 1);
    } else {
        m_items.move(currentIndex, newIndex);
    }

    endMoveRows();
}

int RosterModel::positionToAdd(const RosterItem &item)
{
    for (int i = 0; i < m_items.size(); i++) {
        if (item <= m_items.at(i)) {
            return i;
        }
    }

    // If the item to be positioned is greater than all other items, it is appended to the list.
    return m_items.size();
}

int RosterModel::positionToMove(int currentIndex)
{
    const auto &item = m_items.at(currentIndex);

    for (int i = 0; i < m_items.size(); i++) {
        // In some cases, it is needed to skip the item that is being positioned.
        // Especially, when the item is at the beginning or at the end of the list, it must not
        // be compared to itself in order to find the correct position.
        if (currentIndex != i) {
            if (item <= m_items.at(i)) {
                if (i == currentIndex + 1) {
                    return currentIndex;
                }

                return i;
            }
        }
    }

    // If the item to be positioned is the last item but cannot be positioned somewhere else, its
    // position is not changed.
    // In all other cases, the item is appended to the list.
    return currentIndex == m_items.size() - 1 ? currentIndex : m_items.size();
}

QString RosterModel::formatLastMessageDateTime(const QDateTime &lastMessageDateTime) const
{
    const QDateTime &lastMessageLocalDateTime{lastMessageDateTime.toLocalTime()};

    if (const auto elapsedNightCount = lastMessageDateTime.daysTo(QDateTime::currentDateTimeUtc()); elapsedNightCount == 0) {
        // Today: Return only the time.
        return QLocale::system().toString(lastMessageLocalDateTime.time(), QLocale::ShortFormat);
    } else if (elapsedNightCount == 1) {
        // Yesterday: Return that term.
        return tr("Yesterday");
    } else if (elapsedNightCount <= 7) {
        // Between yesterday and seven days before today: Return the day of the week.
        return QLocale::system().dayName(lastMessageLocalDateTime.date().dayOfWeek(), QLocale::ShortFormat);
    } else {
        // Older than seven days before today: Return the date.
        return QLocale::system().toString(lastMessageLocalDateTime.date(), QLocale::ShortFormat);
    }
}

QString RosterModel::determineGroupChatSenderName(const Message &message) const
{
    const auto groupChatSenderJid = message.groupChatSenderJid;

    if (!groupChatSenderJid.isEmpty()) {
        if (const auto item = findItem(groupChatSenderJid)) {
            return item->displayName();
        }
    }

    return message.groupChatSenderName;
}

#include "moc_RosterModel.cpp"
