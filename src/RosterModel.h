// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
struct Message;
enum class MessageOrigin : quint8;

class RosterModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QStringList accountJids READ accountJids NOTIFY accountJidsChanged)
	Q_PROPERTY(QStringList groups READ groups NOTIFY groupsChanged)

public:
	enum RosterItemRoles {
		AccountJidRole,
		JidRole,
		NameRole,
		GroupsRole,
		LastMessageDateTimeRole,
		UnreadMessagesRole,
		LastMessageRole,
		LastMessageIsDraftRole,
		LastMessageSenderIdRole,
		PinnedRole,
		NotificationsMutedRole,
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
	~RosterModel() override;

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
	 * Returns the account JIDs of all roster items.
	 *
	 * @return all account JIDs
	 */
	QStringList accountJids() const;
	Q_SIGNAL void accountJidsChanged();

	/**
	 * Returns the roster groups of all roster items.
	 *
	 * @return all roster groups
	 */
	QStringList groups() const;
	Q_SIGNAL void groupsChanged();

	Q_INVOKABLE void updateGroup(const QString &oldGroup, const QString &newGroup);
	Q_INVOKABLE void removeGroup(const QString &group);

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
	Q_INVOKABLE void setItemEncryption(const QString &accountJid, Encryption::Enum encryption);

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
	 * Sends read markers for all roster items that have unsent (pending) ones.
	 *
	 * This method must only be called while being logged in.
	 * Otherwise, the pending states of the read markers would be reset even if the pending read
	 * markers could not be sent.
	 *
	 * @param accountJid bare JID of the user's account
	 */
	void sendPendingReadMarkers(const QString &accountJid);

	/**
	 * Searches for the roster item with a given JID.
	 */
	std::optional<RosterItem> findItem(const QString &jid) const;

	const QVector<RosterItem> &items() const;

	void updateItem(const QString &jid, const std::function<void (RosterItem &)> &updateItem);

	Q_INVOKABLE void pinItem(const QString &accountJid, const QString &jid);
	Q_INVOKABLE void unpinItem(const QString &accountJid, const QString &jid);
	Q_INVOKABLE void reorderPinnedItem(const QString &accountJid, const QString &jid, int oldIndex, int newIndex);

	Q_INVOKABLE void setChatStateSendingEnabled(const QString &accountJid, const QString &jid, bool chatStateSendingEnabled);
	Q_INVOKABLE void setReadMarkerSendingEnabled(const QString &accountJid, const QString &jid, bool readMarkerSendingEnabled);
	Q_INVOKABLE void setNotificationsMuted(const QString &accountJid, const QString &jid, bool notificationsMuted);
	Q_INVOKABLE void setAutomaticMediaDownloadsRule(const QString &accountJid, const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule);

signals:
	void addItemRequested(const RosterItem &item);
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

private:
	void handleItemsFetched(const QVector<RosterItem> &items);

	void addItem(const RosterItem &item);
	void replaceItems(const QHash<QString, RosterItem> &items);

	void updateLastMessage(QVector<RosterItem>::Iterator &itr,
						   const Message &message,
						   QVector<int> &changedRoles,
						   bool onlyUpdateIfNewerOrAtSameAge = true);

	/**
	 * Removes all roster items of an account or a specific roster item.
	 *
	 * @param accountJid JID of the account whose roster items are being removed
	 * @param jid JID of the roster item being removed (optional)
	 */
	void removeItems(const QString &accountJid, const QString &jid = {});

	void handleMessageAdded(const Message &message, MessageOrigin origin);
	void handleMessageUpdated(const Message &message);
	void handleDraftMessageAdded(const Message &message);
	void handleDraftMessageUpdated(const Message &message);
	void handleDraftMessageRemoved(const Message &newLastMessage);
	void handleMessageRemoved(const Message &newLastMessage);

	void insertItem(int index, const RosterItem &item);
	void updateItemPosition(int currentIndex);
	int positionToAdd(const RosterItem &item);
	int positionToMove(int currentIndex);
	
	QString formatLastMessageDateTime(const QDateTime &lastMessageDateTime) const;

	QVector<RosterItem> m_items;

	static RosterModel *s_instance;
};
