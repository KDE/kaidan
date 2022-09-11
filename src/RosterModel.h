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

#pragma once

// std
#include <optional>
// Qt
#include <QAbstractListModel>
#include <QFuture>
#include <QVector>
// Kaidan
#include "RosterItem.h"

class Kaidan;
class MessageModel;
class Message;
enum class MessageOrigin : quint8;

class RosterModel : public QAbstractListModel
{
	Q_OBJECT

public:
	enum RosterItemRoles {
		JidRole,
		NameRole,
		LastExchangedRole,
		UnreadMessagesRole,
		LastMessageRole,
	};

	/**
	 * Result for adding a contact by an XMPP URI specifying how the URI is used
	 */
	enum AddContactByUriResult {
		AddingContact,  ///< The contact is being added to the roster.
		ContactExists,  ///< The contact is already in the roster.
		InvalidUri      ///< The URI cannot be used for contact addition.
	};
	Q_ENUM(AddContactByUriResult)

	static RosterModel *instance();

	RosterModel(QObject *parent = nullptr);

	Q_REQUIRED_RESULT bool isEmpty() const;
	Q_REQUIRED_RESULT int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	Q_REQUIRED_RESULT QHash<int, QByteArray> roleNames() const override;
	Q_REQUIRED_RESULT QVariant data(const QModelIndex &index, int role) const override;

	/**
	 * Returns whether this model has a roster item with the passed properties.
	 *
	 * @param jid JID of the roster item
	 *
	 * @return true if a roster item with the passed properties exists, otherwise false
	 */
	Q_INVOKABLE bool hasItem(const QString &jid) const;

	/**
	 * Returns whether an account's presence is subscribed by a roster item.
	 *
	 * @param accountJid JID of the account whose roster item's presence subcription is requested
	 * @param jid JID of the roster item
	 *
	 * @return whether the presence is subscribed by the item
	 */
	bool isPresenceSubscribedByItem(const QString &accountJid, const QString &jid) const;

	std::optional<Encryption::Enum> itemEncryption(const QString &accountJid, const QString &jid) const;
	void setItemEncryption(const QString &accountJid, const QString &jid, Encryption::Enum encryption);

	/**
	 * Adds a contact (bare JID) by a given XMPP URI (e.g., from a scanned QR
	 * code) such as "xmpp:user@example.org".
	 *
	 * @param uriString XMPP URI string that contains only a JID
	 */
	Q_INVOKABLE RosterModel::AddContactByUriResult addContactByUri(const QString &uriString);

	QString lastReadOwnMessageId(const QString &accountJid, const QString &jid) const;
	QString lastReadContactMessageId(const QString &accountJid, const QString &jid) const;

	/**
	 * Searches for the roster item with a given JID.
	 */
	std::optional<RosterItem> findItem(const QString &jid) const;

signals:
	void addItemRequested(const RosterItem &item);
	void removeItemRequested(const QString &jid);
	void updateItemRequested(const QString &jid,
	                         const std::function<void (RosterItem &)> &updateItem);
	void replaceItemsRequested(const QHash<QString, RosterItem> &items);

	/**
	 * Emitted to remove all roster items of an account or a specific roster item.
	 *
	 * @param accountJid JID of the account whose roster items are being removed
	 * @param jid JID of the roster item being removed (optional)
	 */
	void removeItemsRequested(const QString &accountJid, const QString &jid = {});

	/**
	 * Emitted, whan a subscription request was received
	 */
	void subscriptionRequestReceived(const QString &from, const QString &msg);

private slots:
	void handleItemsFetched(const QVector<RosterItem> &items);

	void addItem(const RosterItem &item);
	void removeItem(const QString &jid);
	void updateItem(const QString &jid,
	                const std::function<void (RosterItem &)> &updateItem);
	void replaceItems(const QHash<QString, RosterItem> &items);

	/**
	 * Removes all roster items of an account or a specific roster item.
	 *
	 * @param accountJid JID of the account whose roster items are being removed
	 * @param jid JID of the roster item being removed (optional)
	 */
	void removeItems(const QString &accountJid, const QString &jid = {});

	void handleMessageAdded(const Message &message, MessageOrigin origin);

private:
	void insertItem(int index, const RosterItem &item);
	int updateItemPosition(int currentIndex);
	int positionToInsert(const RosterItem &item);

	QVector<RosterItem> m_items;

	static RosterModel *s_instance;
};
