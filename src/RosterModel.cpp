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

	connect(AccountManager::instance(), &AccountManager::jidChanged, this, [this] {
		beginResetModel();
		m_items.clear();
		endResetModel();

		await(RosterDb::instance()->fetchItems(AccountManager::instance()->jid()), this, [this](QVector<RosterItem> items) {
			handleItemsFetched(items);
		});
	});

	connect(this, &RosterModel::removeItemsRequested, this, [this](const QString &accountJid, const QString &chatJid) {
		RosterDb::instance()->removeItems(accountJid, chatJid);
		removeItems(accountJid, chatJid);

		if (accountJid == MessageModel::instance()->currentAccountJid() && chatJid == MessageModel::instance()->currentChatJid())
			emit Kaidan::instance()->openChatViewRequested();
	});
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
	roles[JidRole] = "jid";
	roles[NameRole] = "name";
	roles[LastExchangedRole] = "lastExchanged";
	roles[UnreadMessagesRole] = "unreadMessages";
	roles[LastMessageRole] = "lastMessage";
	roles[PinnedRole] = "pinned";
	return roles;
}

QVariant RosterModel::data(const QModelIndex &index, int role) const
{
	if (!hasIndex(index.row(), index.column(), index.parent())) {
		qWarning() << "Could not get data from roster model." << index << role;
		return {};
	}

	switch (role) {
	case JidRole:
		return m_items.at(index.row()).jid;
	case NameRole:
		return m_items.at(index.row()).name;
	case LastExchangedRole:
		return m_items.at(index.row()).lastExchanged;
	case UnreadMessagesRole:
		return m_items.at(index.row()).unreadMessages;
	case LastMessageRole:
		return m_items.at(index.row()).lastMessage;
	case PinnedRole:
		return m_items.at(index.row()).pinningPosition >= 0;
	}
	return {};
}

bool RosterModel::hasItem(const QString &jid) const
{
	return findItem(jid).has_value();
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
	emit updateItemRequested(jid, [encryption](RosterItem &item) {
		item.encryption = encryption;
	});
}

RosterModel::AddContactByUriResult RosterModel::addContactByUri(const QString &uriString)
{
	if (QXmppUri::isXmppUri(uriString)) {
		auto uri = QXmppUri(uriString);
		auto jid = uri.jid();

		if (jid.isEmpty()) {
			return AddContactByUriResult::InvalidUri;
		}

		if (RosterModel::instance()->hasItem(jid)) {
			emit Kaidan::instance()->openChatPageRequested(AccountManager::instance()->jid(), jid);
			return AddContactByUriResult::ContactExists;
		}

		emit Kaidan::instance()->client()->rosterManager()->addContactRequested(jid);

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
	for (const auto &item : m_items) {
		if (const auto messageId = item.lastReadContactMessageId; item.readMarkerPending && !messageId.isEmpty()) {
			if (Enums::ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
				const auto chatJid = item.jid;
				runOnThread(Kaidan::instance()->client()->messageHandler(), [chatJid, messageId]() {
					Kaidan::instance()->client()->messageHandler()->sendReadMarker(chatJid, messageId);
				});

				emit updateItemRequested(chatJid, [](RosterItem &item) {
					item.readMarkerPending = false;
				});
			}
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
}

void RosterModel::addItem(const RosterItem &item)
{
	insertItem(positionToInsert(item), item);
}

void RosterModel::removeItem(const QString &jid)
{
	QMutableVectorIterator<RosterItem> itr(m_items);
	int i = 0;
	while (itr.hasNext()) {
		if (itr.next().jid == jid) {
			beginRemoveRows(QModelIndex(), i, i);
			itr.remove();
			endRemoveRows();
			RosterItemNotifier::instance().notifyWatchers(jid, std::nullopt);
			return;
		}
		i++;
	}
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

			m_items.replace(i, item);

			// item was changed: refresh all roles
			emit dataChanged(index(i), index(i), {});
			RosterItemNotifier::instance().notifyWatchers(jid, item);

			// check, if the position of the new item may be different
			updateItemPosition(i);
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
			m_items.begin(),
			m_items.end(),
			[&] (const RosterItem &oldItem) {
				return oldItem.jid == item.jid;
			}
		);

		// use the old item's values, if found
		if (oldItem != m_items.end()) {
			item.lastMessage = oldItem->lastMessage;
			item.lastExchanged = oldItem->lastExchanged;
			item.unreadMessages = oldItem->unreadMessages;
			item.lastReadOwnMessageId = oldItem->lastReadOwnMessageId;
			item.lastReadContactMessageId = oldItem->lastReadContactMessageId;
			item.encryption = oldItem->encryption;
			item.pinningPosition = oldItem->pinningPosition;
		}

		newItems << item;
	}

	// replace all items
	handleItemsFetched(newItems);
}

void RosterModel::pinItem(const QString &, const QString &jid)
{
	emit updateItemRequested(jid, [highestPinningPosition = m_items.at(0).pinningPosition](RosterItem &item) {
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
				emit updateItemRequested(jid, [pinningPosition = item.pinningPosition](RosterItem &item) {
					item.pinningPosition = pinningPosition - 1;
				});
			}
		}

		// Reset the pinning position of the item being unpinned.
		emit updateItemRequested(jid, [](RosterItem &item) {
			item.pinningPosition = -1;
		});
	}
}

void RosterModel::removeItems(const QString &accountJid, const QString &jid)
{
	for (int i = 0; i < m_items.size(); i++) {
		RosterItem &item = m_items[i];

		if (AccountManager::instance()->jid() == accountJid && (item.jid.isEmpty() || item.jid == jid)) {
			beginRemoveRows(QModelIndex(), i, i);
			m_items.remove(i);
			endRemoveRows();
			RosterItemNotifier::instance().notifyWatchers(jid, std::nullopt);
			return;
		}
	}
}

void RosterModel::handleMessageAdded(const Message &message, MessageOrigin origin)
{
	const auto contactJid = message.isOwn ? message.to : message.from;
	auto itr = std::find_if(m_items.begin(), m_items.end(), [&contactJid](const RosterItem &item) {
		return item.jid == contactJid;
	});

	// contact not found
	if (itr == m_items.end())
		return;

	// only set new message if it's newer
	// allow setting old message if the current message is empty
	if (!itr->lastMessage.isEmpty() && itr->lastExchanged >= message.stamp)
		return;

	QVector<int> changedRoles = {
		int(LastExchangedRole)
	};

	// last exchanged
	itr->lastExchanged = message.stamp;

	// last message
	const auto lastMessage = message.previewText();
	if (itr->lastMessage != lastMessage) {
		itr->lastMessage = lastMessage;
		changedRoles << int(LastMessageRole);
	}

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

	if (newUnreadMessages.has_value()) {
		itr->unreadMessages = *newUnreadMessages;
		changedRoles << int(UnreadMessagesRole);

		RosterDb::instance()->updateItem(contactJid, [newCount = *newUnreadMessages](RosterItem &item) {
			item.unreadMessages = newCount;
		});
	}

	// notify gui
	const auto i = std::distance(m_items.begin(), itr);
	const auto modelIndex = index(i);
	emit dataChanged(modelIndex, modelIndex, changedRoles);
	RosterItemNotifier::instance().notifyWatchers(itr->jid, *itr);

	// move row to correct position
	updateItemPosition(i);
}

void RosterModel::insertItem(int index, const RosterItem &item)
{
	beginInsertRows(QModelIndex(), index, index);
	m_items.insert(index, item);
	endInsertRows();
	RosterItemNotifier::instance().notifyWatchers(item.jid, item);
}

void RosterModel::updateItemPosition(int currentPosition)
{
	int newPosition = positionToInsert(m_items.at(currentPosition), currentPosition);
	if (currentPosition == newPosition)
		return;

	beginMoveRows(QModelIndex(), currentPosition, currentPosition, QModelIndex(), newPosition);

	// Cover both cases:
	// 1. Moving to a higher index
	// 2. Moving to a lower index
	if (currentPosition < newPosition) {
		m_items.move(currentPosition, newPosition - 1);
	} else {
		m_items.move(currentPosition, newPosition);
	}

	endMoveRows();
}

int RosterModel::positionToInsert(const RosterItem &item, int skippedIndex)
{
	for (int i = 0; i < m_items.size(); i++) {
		// In some cases, it is needed to skip the item that is being positioned.
		// Especially, when the item is at the beginning or at the end of the list, it must not
		// be compared to itself in order to find the correct position.
		if (skippedIndex != i) {
			if (item <= m_items.at(i)) {
				if (i == skippedIndex + 1) {
					return skippedIndex;
				}

				return i;
			}
		}
	}

	// If the item to be positioned is the last item but cannot be positioned somewhere else, its
	// position is not changed.
	// In all other cases, the item is being appended to the list.
	return skippedIndex == m_items.size() - 1 ? skippedIndex : m_items.size();
}
