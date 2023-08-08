// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterDb.h"
// Kaidan
#include "Globals.h"
#include "SqlUtils.h"
#include "RosterItem.h"
#include "Message.h"
#include "MessageDb.h"
// Qt
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>

using namespace SqlUtils;

RosterDb *RosterDb::s_instance = nullptr;

RosterDb::RosterDb(Database *db, QObject *parent)
        : DatabaseComponent(db, parent)
{
	Q_ASSERT(!RosterDb::s_instance);
	s_instance = this;
}

RosterDb::~RosterDb()
{
	s_instance = nullptr;
}

RosterDb *RosterDb::instance()
{
	return s_instance;
}

void RosterDb::parseItemsFromQuery(QSqlQuery &query, QVector<RosterItem> &items)
{
	QSqlRecord rec = query.record();
	int idxAccountJid = rec.indexOf("accountJid");
	int idxJid = rec.indexOf("jid");
	int idxName = rec.indexOf("name");
	int idxSubscription = rec.indexOf("subscription");
	int idxEncryption = rec.indexOf("encryption");
	int idxUnreadMessages = rec.indexOf("unreadMessages");
	int idxLastReadOwnMessageId = rec.indexOf("lastReadOwnMessageId");
	int idxLastReadContactMessageId = rec.indexOf("lastReadContactMessageId");
	int idxReadMarkerPending = rec.indexOf("readMarkerPending");
	int idxPinningPosition = rec.indexOf("pinningPosition");
	int idxChateStateSendingEnabled = rec.indexOf("chatStateSendingEnabled");
	int idxReadMarkerSendingEnabled = rec.indexOf("readMarkerSendingEnabled");
	int idxDraftMessageId = rec.indexOf("draftMessageId");
	int idxNotificationsMuted = rec.indexOf("notificationsMuted");

	while (query.next()) {
		RosterItem item;
		item.accountJid = query.value(idxAccountJid).toString();
		item.jid = query.value(idxJid).toString();
		item.name = query.value(idxName).toString();
		item.subscription = QXmppRosterIq::Item::SubscriptionType(query.value(idxSubscription).toInt());
		item.encryption = Encryption::Enum(query.value(idxEncryption).toInt());
		item.unreadMessages = query.value(idxUnreadMessages).toInt();
		item.lastReadOwnMessageId = query.value(idxLastReadOwnMessageId).toString();
		item.lastReadContactMessageId = query.value(idxLastReadContactMessageId).toString();
		item.readMarkerPending = query.value(idxReadMarkerPending).toBool();
		item.pinningPosition = query.value(idxPinningPosition).toInt();
		item.chatStateSendingEnabled = query.value(idxChateStateSendingEnabled).toBool();
		item.readMarkerSendingEnabled = query.value(idxReadMarkerSendingEnabled).toBool();
		item.draftMessageId = query.value(idxDraftMessageId).toString();
		item.notificationsMuted = query.value(idxNotificationsMuted).toBool();

		items << std::move(item);
	}
}

QSqlRecord RosterDb::createUpdateRecord(const RosterItem &oldItem, const RosterItem &newItem)
{
	QSqlRecord rec;
	if (oldItem.accountJid != newItem.accountJid)
		rec.append(createSqlField("accountJid", newItem.accountJid));
	if (oldItem.jid != newItem.jid)
		rec.append(createSqlField("jid", newItem.jid));
	if (oldItem.name != newItem.name)
		rec.append(createSqlField("name", newItem.name));
	if (oldItem.subscription != newItem.subscription)
		rec.append(createSqlField("subscription", newItem.subscription));
	if (oldItem.encryption != newItem.encryption)
		rec.append(createSqlField("encryption", newItem.encryption));
	if (oldItem.unreadMessages != newItem.unreadMessages)
		rec.append(createSqlField("unreadMessages", newItem.unreadMessages));
	if (oldItem.lastReadOwnMessageId != newItem.lastReadOwnMessageId)
		rec.append(createSqlField("lastReadOwnMessageId", newItem.lastReadOwnMessageId));
	if (oldItem.lastReadContactMessageId != newItem.lastReadContactMessageId)
		rec.append(createSqlField("lastReadContactMessageId", newItem.lastReadContactMessageId));
	if (oldItem.readMarkerPending != newItem.readMarkerPending)
		rec.append(createSqlField("readMarkerPending", newItem.readMarkerPending));
	if(oldItem.pinningPosition != newItem.pinningPosition)
		rec.append(createSqlField("pinningPosition", newItem.pinningPosition));
	if (oldItem.chatStateSendingEnabled != newItem.chatStateSendingEnabled)
		rec.append(createSqlField("chatStateSendingEnabled", newItem.chatStateSendingEnabled));
	if (oldItem.readMarkerSendingEnabled != newItem.readMarkerSendingEnabled)
		rec.append(createSqlField("readMarkerSendingEnabled", newItem.readMarkerSendingEnabled));
	if (oldItem.draftMessageId != newItem.draftMessageId)
		rec.append(createSqlField("draftMessageId", newItem.draftMessageId));
	if (oldItem.notificationsMuted != newItem.notificationsMuted)
		rec.append(createSqlField("notificationsMuted", newItem.notificationsMuted));

	return rec;
}

QFuture<void> RosterDb::addItem(const RosterItem &item)
{
	return addItems({item});
}

QFuture<void> RosterDb::addItems(const QVector<RosterItem> &items)
{
	return run([this, items]() {
		auto query = createQuery();
		transaction();

		prepareQuery(query, sqlDriver().sqlStatement(
			QSqlDriver::InsertStatement,
			DB_TABLE_ROSTER,
			sqlRecord(DB_TABLE_ROSTER),
			true
		));

		for (const auto &item : items) {
			query.addBindValue(item.accountJid);
			query.addBindValue(item.jid);
			query.addBindValue(item.name);
			query.addBindValue(item.subscription);
			query.addBindValue(item.encryption);
			query.addBindValue(item.unreadMessages);
			query.addBindValue(QString()); // lastReadOwnMessageId
			query.addBindValue(QString()); // lastReadContactMessageId
			query.addBindValue(item.readMarkerPending);
			query.addBindValue(item.pinningPosition);
			query.addBindValue(item.chatStateSendingEnabled);
			query.addBindValue(item.readMarkerSendingEnabled);
			query.addBindValue(QString()); // draftMessageId
			query.addBindValue(item.notificationsMuted);
			execQuery(query);

			addGroups(item.accountJid, item.jid, item.groups);
		}

		commit();
	});
}

QFuture<void> RosterDb::updateItem(const QString &jid,
			  const std::function<void (RosterItem &)> &updateItem)
{
	return run([this, jid, updateItem]() {
		// load current roster item from db
		auto query = createQuery();
		execQuery(query, "SELECT * FROM roster WHERE jid = ? LIMIT 1", {jid});

		QVector<RosterItem> items;
		parseItemsFromQuery(query, items);
		fetchGroups(items);

		// update loaded item
		if (!items.isEmpty()) {
			const auto &oldItem = items.first();
			RosterItem newItem = oldItem;
			updateItem(newItem);

			// Replace the old item's values with the updated ones if the item has changed.
			if (oldItem != newItem) {
				updateGroups(oldItem, newItem);

				if (auto record = createUpdateRecord(oldItem, newItem); !record.isEmpty()) {
					// Create an SQL record containing only the differences.
					updateItemByRecord(jid, record);
				}
			}
		}
	});
}

QFuture<void> RosterDb::replaceItems(const QHash<QString, RosterItem> &items)
{
	return run([this, items]() {
		// load current items
		auto query = createQuery();
		execQuery(query, "SELECT * FROM roster");

		QVector<RosterItem> currentItems;
		parseItemsFromQuery(query, currentItems);
		fetchGroups(currentItems);

		transaction();

		QList<QString> keys = items.keys();
		QSet<QString> newJids = QSet<QString>(keys.begin(), keys.end());

		for (const auto &oldItem : qAsConst(currentItems)) {
			// We will remove the already existing JIDs, so we get a set of JIDs that
			// are completely new.
			//
			// By calling remove(), we also find out whether the JID is already
			// existing or not.
			if (newJids.remove(oldItem.jid)) {
				// item is also included in newJids -> update
				replaceItem(oldItem, items[oldItem.jid]);
			} else {
				// item is not included in newJids -> delete
				removeItems({}, oldItem.jid);
			}
		}

		// now add the completely new JIDs
		for (const QString &jid : newJids) {
			addItem(items[jid]);
		}

		commit();
	});
}

QFuture<void> RosterDb::removeItems(const QString &accountJid, const QString &jid)
{
	return run([this, accountJid, jid]() {
		auto query = createQuery();

		if (jid.isEmpty()) {
			execQuery(
				query,
				"DELETE FROM " DB_TABLE_ROSTER " "
				"WHERE accountJid = :accountJid",
				std::vector<QueryBindValue> { { u":accountJid", accountJid } }
			);

			removeGroups(accountJid);
		} else {
			execQuery(
				query,
				"DELETE FROM " DB_TABLE_ROSTER " "
				"WHERE accountJid = :accountJid AND jid = :jid",
				{ { u":accountJid", accountJid },
				  { u":jid", jid } }
			);

			removeGroups(accountJid, jid);
		}
	});
}

QFuture<void> RosterDb::replaceItem(const RosterItem &oldItem, const RosterItem &newItem)
{
	return run([this, oldItem, newItem]() {
		QSqlRecord record;

		if (oldItem.name != newItem.name) {
			record.append(createSqlField("name", newItem.name));
		}
		if (oldItem.subscription != newItem.subscription) {
			record.append(createSqlField("subscription", newItem.subscription));
		}
		if (oldItem.groups != newItem.groups) {
			updateGroups(oldItem, newItem);
		}

		if (!record.isEmpty()) {
			updateItemByRecord(oldItem.jid, record);
		}
	});
}

QFuture<QVector<RosterItem>> RosterDb::fetchItems(const QString &accountId)
{
	return run([this, accountId]() {
		auto query = createQuery();
		execQuery(query, "SELECT * FROM roster");

		QVector<RosterItem> items;
		parseItemsFromQuery(query, items);

		for (auto &item : items) {
			Message lastMessage = MessageDb::instance()->_fetchLastMessage(accountId, item.jid);
			item.lastMessageDateTime = lastMessage.stamp;
			item.lastMessage = lastMessage.previewText();
		}

		fetchGroups(items);

		return items;
	});
}

void RosterDb::updateItemByRecord(const QString &jid, const QSqlRecord &record)
{
	auto query = createQuery();
	auto &driver = sqlDriver();

	QMap<QString, QVariant> keyValuePairs = {
		{ "jid", jid }
	};

	execQuery(
		query,
		driver.sqlStatement(
			QSqlDriver::UpdateStatement,
			DB_TABLE_ROSTER,
			record,
			false
		) +
		simpleWhereStatement(&driver, keyValuePairs)
				);
}

void RosterDb::fetchGroups(QVector<RosterItem> &items)
{
	enum { Group };
	auto query = createQuery();

	for(auto &item : items) {
		execQuery(
			query,
			"SELECT name FROM rosterGroups "
			"WHERE accountJid = ? AND chatJid = ?",
			{ item.accountJid, item.jid }
		);

		// Iterate over all found groups.
		while (query.next()) {
			auto &groups = item.groups;
			groups.append(query.value(Group).toString());
		}
	}
}

void RosterDb::addGroups(const QString &accountJid, const QString &jid, const QVector<QString> &groups)
{
	auto query = createQuery();

	for (const auto &group : groups) {
		execQuery(
			query,
			"INSERT OR IGNORE INTO " DB_TABLE_ROSTER_GROUPS
			"(accountJid, chatJid, name) VALUES(:accountJid, :chatJid, :name)",
			{ { u":accountJid", accountJid },
			  { u":chatJid", jid },
			  { u":name", group } }
		);
	}
}

void RosterDb::updateGroups(const RosterItem &oldItem, const RosterItem &newItem)
{
	const auto &oldGroups = oldItem.groups;

	if(const auto &newGroups = newItem.groups; oldGroups != newGroups) {
		auto query = createQuery();

		// Remove old groups.
		for(auto itr = oldGroups.begin(); itr != oldGroups.end(); ++itr) {
			const auto group = *itr;

			if(!newGroups.contains(group)) {
				execQuery(
					query,
					"DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
					"WHERE accountJid = :accountJid AND chatJid = :chatJid AND name = :name",
					{ { u":accountJid", oldItem.accountJid },
					  { u":chatJid", oldItem.jid },
					  { u":name", group } }
				);
			}
		}

		// Add new groups.
		for(auto itr = newGroups.begin(); itr != newGroups.end(); ++itr) {
			const auto group = *itr;

			if(!oldGroups.contains(group)) {
				execQuery(
					query,
					"INSERT or IGNORE INTO " DB_TABLE_ROSTER_GROUPS " "
					"(accountJid, chatJid, name) "
					"VALUES (:accountJid, :chatJid, :name)",
					{ { u":accountJid", oldItem.accountJid },
					  { u":chatJid", oldItem.jid },
					  { u":name", group } }
				);
			}
		}
	}
}

void RosterDb::removeGroups(const QString &accountJid)
{
	auto query = createQuery();

	execQuery(
		query,
		"DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
		"WHERE accountJid = :accountJid",
		std::vector<QueryBindValue> { { u":accountJid", accountJid } }
	);
}

void RosterDb::removeGroups(const QString &accountJid, const QString &jid)
{
	auto query = createQuery();

	execQuery(
		query,
		"DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
		"WHERE accountJid = :accountJid AND chatJid = :chatJid",
		{ { u":accountJid", accountJid },
		  { u":chatJid", jid } }
	);
}
