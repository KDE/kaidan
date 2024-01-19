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

// Kaidan
#include "AccountManager.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "MessageHandler.h"
#include "MessageModel.h"
#include "RosterDb.h"
#include "RosterItemWatcher.h"
#include "RosterManager.h"

#include "qxmpp-exts/QXmppUri.h"

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

	connect(this, &RosterModel::addItemRequested, this, &RosterModel::addItem);
	connect(this, &RosterModel::addItemRequested, RosterDb::instance(), &RosterDb::addItem);

	connect(this, &RosterModel::updateItemRequested,
	        this, &RosterModel::updateItem);
	connect(this, &RosterModel::updateItemRequested, RosterDb::instance(), &RosterDb::updateItem);

	connect(this, &RosterModel::replaceItemsRequested,
	        this, &RosterModel::replaceItems);
	connect(this, &RosterModel::replaceItemsRequested, RosterDb::instance(), &RosterDb::replaceItems);

	connect(MessageDb::instance(), &MessageDb::messageAdded,
	        this, &RosterModel::handleMessageAdded);
	connect(MessageDb::instance(), &MessageDb::messageUpdated, this, &RosterModel::handleMessageUpdated);
	connect(MessageDb::instance(), &MessageDb::draftMessageAdded, this, &RosterModel::handleDraftMessageAdded);
	connect(MessageDb::instance(), &MessageDb::draftMessageUpdated, this, &RosterModel::handleDraftMessageUpdated);
	connect(MessageDb::instance(), &MessageDb::draftMessageRemoved, this, &RosterModel::handleDraftMessageRemoved);
	connect(MessageDb::instance(), &MessageDb::messageRemoved, this, &RosterModel::handleMessageRemoved);

	connect(AccountManager::instance(), &AccountManager::jidChanged, this, [this] {
		beginResetModel();
		m_items.clear();
		endResetModel();

		await(RosterDb::instance()->fetchItems(), this, [this](const QVector<RosterItem> &items) {
			handleItemsFetched(items);
		});
	});

	connect(this, &RosterModel::removeItemsRequested, this, [this](const QString &accountJid, const QString &chatJid) {
		RosterDb::instance()->removeItems(accountJid, chatJid);
		removeItems(accountJid, chatJid);

		if (accountJid == MessageModel::instance()->currentAccountJid() && chatJid == MessageModel::instance()->currentChatJid()) {
			Q_EMIT Kaidan::instance()->closeChatPageRequested();
		}
	});
}

RosterModel::~RosterModel()
{
	s_instance = nullptr;
}

bool RosterModel::isEmpty() const
{
	return m_items.isEmpty();
}

int RosterModel::rowCount(const QModelIndex&) const
{
	return m_items.length();
}

QHash<int, QByteArray> RosterModel::roleNames() const
{
	QHash<int, QByteArray> roles;
	roles[AccountJidRole] = "accountJid";
	roles[JidRole] = "jid";
	roles[NameRole] = "name";
	roles[GroupsRole] = "groups";
	roles[LastMessageDateTimeRole] = "lastMessageDateTime";
	roles[UnreadMessagesRole] = "unreadMessages";
	roles[LastMessageRole] = "lastMessage";
	roles[LastMessageIsDraftRole] = "lastMessageIsDraft";
	roles[LastMessageSenderIdRole] = "lastMessageSenderId";
	roles[PinnedRole] = "pinned";
	roles[NotificationsMutedRole] = "notificationsMuted";
	return roles;
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		qWarning() << "Could not get data from roster model." << index << role;
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
	case LastMessageSenderIdRole:
		return item.lastMessageSenderId;
	case PinnedRole:
		return item.pinningPosition >= 0;
	case NotificationsMutedRole:
		return item.notificationsMuted;
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

std::optional<RosterItem> RosterModel::findItem(const QString &jid) const
{
	for (const auto &item : std::as_const(m_items)) {
		if (item.jid == jid) {
			return item;
		}
	}

	return std::nullopt;
}

const QVector<RosterItem> &RosterModel::items() const
{
	return m_items;
}

bool RosterModel::isPresenceSubscribedByItem(const QString &, const QString &jid) const
{
	if (auto item = findItem(jid)) {
		return item->subscription == QXmppRosterIq::Item::From || item->subscription == QXmppRosterIq::Item::Both ;
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
	Q_EMIT updateItemRequested(jid, [encryption](RosterItem &item) {
		item.encryption = encryption;
	});
}

void RosterModel::setItemEncryption(const QString &, Encryption::Enum encryption)
{
	for (const auto &item : std::as_const(m_items)) {
		Q_EMIT updateItemRequested(item.jid, [encryption](RosterItem &item) {
			item.encryption = encryption;
		});
	}
}

RosterModel::AddContactByUriResult RosterModel::addContactByUri(const QString &accountJid, const QString &uriString)
{
	if (QXmppUri::isXmppUri(uriString)) {
		auto uri = QXmppUri(uriString);
		auto jid = uri.jid();

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
				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid, messageId]() {
					Kaidan::instance()->client()->messageHandler()->sendReadMarker(chatJid, messageId);
				});
			}

			Q_EMIT updateItemRequested(chatJid, [](RosterItem &item) {
				item.readMarkerPending = false;
			});
		}
	}
}

void RosterModel::handleItemsFetched(const QVector<RosterItem> &items)
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

void RosterModel::updateItem(const QString &jid,
                             const std::function<void (RosterItem &)> &updateItem)
{
	for (int i = 0; i < m_items.length(); i++) {
		if (m_items.at(i).jid == jid) {
			// update item
			RosterItem item = m_items.at(i);
			updateItem(item);

			// check if item was actually modified
			if (m_items.at(i) == item)
				return;

			// TODO: Uncomment this and see TODO in ContactDetailsContent once fixed in Kirigami Addons.
//			auto oldGroups = groups();

			m_items.replace(i, item);

			// item was changed: refresh all roles
			Q_EMIT dataChanged(index(i), index(i), {});
			RosterItemNotifier::instance().notifyWatchers(jid, item);

			// check, if the position of the new item may be different
			updateItemPosition(i);

			// TODO: Uncomment this and see TODO in ContactDetailsContent once fixed in Kirigami Addons.
//			if (oldGroups != groups()) {
				Q_EMIT groupsChanged();
//			}

			return;
		}
	}
}

void RosterModel::replaceItems(const QHash<QString, RosterItem> &items)
{
	QVector<RosterItem> newItems;
	for (auto item : items) {
		// find old item
		auto oldItem = std::find_if(
			m_items.cbegin(),
			m_items.cend(),
			[&] (const RosterItem &oldItem) {
				return oldItem.jid == item.jid;
			}
		);

		// use the old item's values, if found
		if (oldItem != m_items.cend()) {
			item.lastMessage = oldItem->lastMessage;
			item.lastMessageDateTime = oldItem->lastMessageDateTime;
			item.unreadMessages = oldItem->unreadMessages;
			item.lastReadOwnMessageId = oldItem->lastReadOwnMessageId;
			item.lastReadContactMessageId = oldItem->lastReadContactMessageId;
			item.encryption = oldItem->encryption;
			item.pinningPosition = oldItem->pinningPosition;
			item.chatStateSendingEnabled = oldItem->chatStateSendingEnabled;
			item.readMarkerSendingEnabled = oldItem->readMarkerSendingEnabled;
			item.notificationsMuted = oldItem->notificationsMuted;
			item.lastMessageDeliveryState = oldItem->lastMessageDeliveryState;
			item.lastMessageSenderId = oldItem->lastMessageSenderId;
			item.automaticMediaDownloadsRule = oldItem->automaticMediaDownloadsRule;
		}

		newItems << item;
	}

	// replace all items
	handleItemsFetched(newItems);
}

void RosterModel::updateLastMessage(
	QVector<RosterItem>::Iterator &itr, const Message &message,
	QVector<int> &changedRoles, bool onlyUpdateIfNewerOrAtSameAge)
{
	// If desired, only set the new message as the current last message if it is newer than
	// the current one or at the same age.
	// That makes it possible to use the previous message as the new last message if the current
	// last message is empty.
	if (!itr->lastMessage.isEmpty() && (onlyUpdateIfNewerOrAtSameAge && itr->lastMessageDateTime > message.timestamp)) {
		return;
	}

	// The new message is only set as the current last message if they are different and there
	// is no draft message.
	if (const auto lastMessage = message.previewText();
		itr->lastMessageDeliveryState != Enums::DeliveryState::Draft && itr->lastMessage != lastMessage)
	{
		itr->lastMessageDateTime = message.timestamp;
		itr->lastMessage = lastMessage;
		itr->lastMessageSenderId = message.senderId;

		changedRoles = {
			int(LastMessageRole),
			int(LastMessageSenderIdRole),
			int(LastMessageDateTimeRole),
		};
	}
}

void RosterModel::pinItem(const QString &, const QString &jid)
{
	Q_EMIT updateItemRequested(jid, [highestPinningPosition = m_items.at(0).pinningPosition](RosterItem &item) {
		item.pinningPosition = highestPinningPosition + 1;
	});
}

void RosterModel::unpinItem(const QString &, const QString &jid)
{
	if (const auto itemBeingUnpinned = findItem(jid)) {
		for (const auto &item : std::as_const(m_items))	{
			// Decrease the pinning position of the pinned items with higher pinning positions than
			// the pinning position of the item being pinned.
			if (item.pinningPosition > itemBeingUnpinned->pinningPosition) {
				Q_EMIT updateItemRequested(item.jid, [](RosterItem &item) {
					item.pinningPosition -= 1;
				});
			}
		}

		// Reset the pinning position of the item being unpinned.
		Q_EMIT updateItemRequested(jid, [](RosterItem &item) {
			item.pinningPosition = -1;
		});
	}
}

void RosterModel::reorderPinnedItem(const QString &, const QString &jid, int oldIndex, int newIndex)
{
	const auto &itemBeingReordered = m_items.at(oldIndex);
	const auto pinningPositionDifference = oldIndex - newIndex;

	const auto oldPinningPosition = itemBeingReordered.pinningPosition;
	const auto newPinningPosition = oldPinningPosition + pinningPositionDifference;

	// Do not reorder anything if the item would go out of the range of the pinned items.
	// That happens when the item is dragged below unpinned items.
	if (newPinningPosition < 0) {
		return;
	}

	// Update the pinning position of the pinned items in between the old and the new pinning
	// position of the item being reordered.
	// "items" must be copied since its elements are changed via "updateItemRequested()".
	const auto items = m_items;
	for (const auto &item : items)	{
		if (item.pinningPosition != -1 && item.jid != itemBeingReordered.jid) {
			const auto pinningPosition = item.pinningPosition;
			const auto itemMovedUpwards = pinningPositionDifference > 0;

			if (itemMovedUpwards && pinningPosition > oldPinningPosition && pinningPosition <= newPinningPosition) {
				Q_EMIT updateItemRequested(item.jid, [](RosterItem &item) {
					--item.pinningPosition;
				});
			} else if (!itemMovedUpwards && pinningPosition < oldPinningPosition && pinningPosition >= newPinningPosition) {
				Q_EMIT updateItemRequested(item.jid, [](RosterItem &item) {
					++item.pinningPosition;
				});
			}
		}
	}

	// Update the pinning position of the reordered item.
	Q_EMIT updateItemRequested(jid, [newPinningPosition](RosterItem &item) {
		item.pinningPosition = newPinningPosition;
	});
}

void RosterModel::setChatStateSendingEnabled(const QString &, const QString &jid, bool chatStateSendingEnabled)
{
	Q_EMIT updateItemRequested(jid, [=](RosterItem &item) {
		item.chatStateSendingEnabled = chatStateSendingEnabled;
	});
}

void RosterModel::setReadMarkerSendingEnabled(const QString &, const QString &jid, bool readMarkerSendingEnabled)
{
	Q_EMIT updateItemRequested(jid, [=](RosterItem &item) {
		item.readMarkerSendingEnabled = readMarkerSendingEnabled;
	});
}

void RosterModel::setNotificationsMuted(const QString &, const QString &jid, bool notificationsMuted)
{
	Q_EMIT updateItemRequested(jid, [=](RosterItem &item) {
		item.notificationsMuted = notificationsMuted;
	});
}

void RosterModel::setAutomaticMediaDownloadsRule(const QString &, const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule)
{
	Q_EMIT updateItemRequested(jid, [rule](RosterItem &item) {
		item.automaticMediaDownloadsRule = rule;
	});
}

void RosterModel::removeItems(const QString &accountJid, const QString &jid)
{
	for (int i = 0; i < m_items.size(); i++) {
		const RosterItem &item = m_items.at(i);

		if (AccountManager::instance()->jid() == accountJid && (item.jid.isEmpty() || item.jid == jid)) {
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

			return;
		}
	}
}

void RosterModel::handleMessageAdded(const Message &message, MessageOrigin origin)
{
	auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
		return item.jid == message.chatJid;
	});

	// contact not found
	if (itr == m_items.end())
		return;

	QVector<int> changedRoles = {
		int(LastMessageDateTimeRole)
	};

	updateLastMessage(itr, message, changedRoles);

	// unread messages counter
	std::optional<int> newUnreadMessages;
	if (message.isOwn()) {
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

	if (newUnreadMessages.has_value()) {
		itr->unreadMessages = *newUnreadMessages;
		changedRoles << int(UnreadMessagesRole);

		RosterDb::instance()->updateItem(message.chatJid, [newCount = *newUnreadMessages](RosterItem &item) {
			item.unreadMessages = newCount;
		});
	}

	// notify gui
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

	// move row to correct position
	updateItemPosition(i);
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

	QVector<int> changedRoles = {
		int(LastMessageRole)
	};

	updateLastMessage(itr, message, changedRoles);

	// Notify the user interface and watchers.
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);
}

void RosterModel::handleDraftMessageAdded(const Message &message)
{
	auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
		return item.jid == message.chatJid;
	});

	// contact not found
	if (itr == m_items.end())
		return;

	QVector<int> changedRoles = {
		int(LastMessageDateTimeRole),
		int(LastMessageRole),
		int(LastMessageIsDraftRole)
	};

	const auto lastMessage = message.previewText();
	itr->lastMessageDateTime = message.timestamp;
	itr->lastMessageDeliveryState = Enums::DeliveryState::Draft;
	itr->lastMessage = lastMessage;

	// notify gui
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

	// Move the updated item to its correct position.
	updateItemPosition(i);
}

void RosterModel::handleDraftMessageUpdated(const Message &message)
{
	auto itr = std::find_if(m_items.begin(), m_items.end(), [&message](const RosterItem &item) {
		return item.jid == message.chatJid;
	});

	// contact not found
	if (itr == m_items.end())
		return;

	QVector<int> changedRoles = {
		int(LastMessageDateTimeRole),
		int(LastMessageRole),
		int(LastMessageIsDraftRole)
	};

	const auto lastMessage = message.previewText();
	itr->lastMessageDateTime = message.timestamp;
	itr->lastMessage = lastMessage;



	// notify gui
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

	// Move the updated item to its correct position.
	updateItemPosition(i);
}

void RosterModel::handleDraftMessageRemoved(const Message &newLastMessage)
{
	auto itr = std::find_if(m_items.begin(), m_items.end(), [&newLastMessage](const RosterItem &item) {
		return item.accountJid == newLastMessage.accountJid && item.jid == newLastMessage.chatJid;
	});

	// contact not found
	if (itr == m_items.end()) {
		return;
	}

	QVector<int> changedRoles = {
		int(LastMessageDateTimeRole),
		int(LastMessageRole),
		int(LastMessageIsDraftRole)
	};

	itr->lastMessageDeliveryState = newLastMessage.deliveryState;
	itr->lastMessageDateTime = newLastMessage.timestamp;
	itr->lastMessage = newLastMessage.body;

	// notify gui
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

	// Move the updated item to its correct position.
	updateItemPosition(i);
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

	QVector<int> changedRoles {};

	updateLastMessage(itr, newLastMessage, changedRoles, false);

	// Notify the user interface and watchers.
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	Q_EMIT dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);
}

void RosterModel::insertItem(int index, const RosterItem &item)
{
	auto newAccountJidAdded = !accountJids().contains(item.accountJid);
	auto oldGroupCount = groups().size();

	beginInsertRows(QModelIndex(), index, index);
	m_items.insert(index, item);
	endInsertRows();

	RosterItemNotifier::instance().notifyWatchers(item.jid, item);

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
	const QDateTime &lastMessageLocalDateTime { lastMessageDateTime.toLocalTime() };

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
