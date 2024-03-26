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

Q_DECLARE_METATYPE(QXmppRosterIq::Item::SubscriptionType)

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
	int idxAccountJid = rec.indexOf(QStringLiteral("accountJid"));
	int idxJid = rec.indexOf(QStringLiteral("jid"));
	int idxName = rec.indexOf(QStringLiteral("name"));
	int idxSubscription = rec.indexOf(QStringLiteral("subscription"));
	int idxEncryption = rec.indexOf(QStringLiteral("encryption"));
	int idxUnreadMessages = rec.indexOf(QStringLiteral("unreadMessages"));
	int idxLastReadOwnMessageId = rec.indexOf(QStringLiteral("lastReadOwnMessageId"));
	int idxLastReadContactMessageId = rec.indexOf(QStringLiteral("lastReadContactMessageId"));
	int idxReadMarkerPending = rec.indexOf(QStringLiteral("readMarkerPending"));
	int idxPinningPosition = rec.indexOf(QStringLiteral("pinningPosition"));
	int idxChateStateSendingEnabled = rec.indexOf(QStringLiteral("chatStateSendingEnabled"));
	int idxReadMarkerSendingEnabled = rec.indexOf(QStringLiteral("readMarkerSendingEnabled"));
	int idxNotificationsMuted = rec.indexOf(QStringLiteral("notificationsMuted"));
	int idxAutomaticMediaDownloadsRule = rec.indexOf(QStringLiteral("automaticMediaDownloadsRule"));

	while (query.next()) {
		RosterItem item;
		item.accountJid = query.value(idxAccountJid).toString();
		item.jid = query.value(idxJid).toString();
		item.name = query.value(idxName).toString();
		item.subscription = query.value(idxSubscription).value<QXmppRosterIq::Item::SubscriptionType>();
		item.encryption = query.value(idxEncryption).value<Encryption::Enum>();
		item.unreadMessages = query.value(idxUnreadMessages).toInt();
		item.lastReadOwnMessageId = query.value(idxLastReadOwnMessageId).toString();
		item.lastReadContactMessageId = query.value(idxLastReadContactMessageId).toString();
		item.readMarkerPending = query.value(idxReadMarkerPending).toBool();
		item.pinningPosition = query.value(idxPinningPosition).toInt();
		item.chatStateSendingEnabled = query.value(idxChateStateSendingEnabled).toBool();
		item.readMarkerSendingEnabled = query.value(idxReadMarkerSendingEnabled).toBool();
		item.notificationsMuted = query.value(idxNotificationsMuted).toBool();
		item.automaticMediaDownloadsRule = query.value(idxAutomaticMediaDownloadsRule).value<RosterItem::AutomaticMediaDownloadsRule>();

		items << std::move(item);
	}
}

QSqlRecord RosterDb::createUpdateRecord(const RosterItem &oldItem, const RosterItem &newItem)
{
	QSqlRecord rec;
	if (oldItem.accountJid != newItem.accountJid)
		rec.append(createSqlField(QStringLiteral("accountJid"), newItem.accountJid));
	if (oldItem.jid != newItem.jid)
		rec.append(createSqlField(QStringLiteral("jid"), newItem.jid));
	if (oldItem.name != newItem.name)
		rec.append(createSqlField(QStringLiteral("name"), newItem.name));
	if (oldItem.subscription != newItem.subscription)
		rec.append(createSqlField(QStringLiteral("subscription"), newItem.subscription));
	if (oldItem.encryption != newItem.encryption)
		rec.append(createSqlField(QStringLiteral("encryption"), newItem.encryption));
	if (oldItem.unreadMessages != newItem.unreadMessages)
		rec.append(createSqlField(QStringLiteral("unreadMessages"), newItem.unreadMessages));
	if (oldItem.lastReadOwnMessageId != newItem.lastReadOwnMessageId)
		rec.append(createSqlField(QStringLiteral("lastReadOwnMessageId"), newItem.lastReadOwnMessageId));
	if (oldItem.lastReadContactMessageId != newItem.lastReadContactMessageId)
		rec.append(createSqlField(QStringLiteral("lastReadContactMessageId"), newItem.lastReadContactMessageId));
	if (oldItem.readMarkerPending != newItem.readMarkerPending)
		rec.append(createSqlField(QStringLiteral("readMarkerPending"), newItem.readMarkerPending));
	if(oldItem.pinningPosition != newItem.pinningPosition)
		rec.append(createSqlField(QStringLiteral("pinningPosition"), newItem.pinningPosition));
	if (oldItem.chatStateSendingEnabled != newItem.chatStateSendingEnabled)
		rec.append(createSqlField(QStringLiteral("chatStateSendingEnabled"), newItem.chatStateSendingEnabled));
	if (oldItem.readMarkerSendingEnabled != newItem.readMarkerSendingEnabled)
		rec.append(createSqlField(QStringLiteral("readMarkerSendingEnabled"), newItem.readMarkerSendingEnabled));
	if (oldItem.notificationsMuted != newItem.notificationsMuted)
		rec.append(createSqlField(QStringLiteral("notificationsMuted"), newItem.notificationsMuted));
	if (oldItem.automaticMediaDownloadsRule != newItem.automaticMediaDownloadsRule)
		rec.append(createSqlField(QStringLiteral("automaticMediaDownloadsRule"), static_cast<int>(newItem.automaticMediaDownloadsRule)));

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
			QStringLiteral(DB_TABLE_ROSTER),
			sqlRecord(QStringLiteral(DB_TABLE_ROSTER)),
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
			query.addBindValue(item.notificationsMuted);
			query.addBindValue(static_cast<int>(item.automaticMediaDownloadsRule));
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
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM roster
				WHERE jid = :jid
				LIMIT 1
			)"),
			{
				{ u":jid", jid },
			}
		);

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
		execQuery(query, QStringLiteral("SELECT * FROM roster"));

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
				QStringLiteral("DELETE FROM " DB_TABLE_ROSTER " "
				"WHERE accountJid = :accountJid"),
				std::vector<QueryBindValue> { { u":accountJid", accountJid } }
			);

			removeGroups(accountJid);
		} else {
			execQuery(
				query,
				QStringLiteral("DELETE FROM " DB_TABLE_ROSTER " "
				"WHERE accountJid = :accountJid AND jid = :jid"),
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
			record.append(createSqlField(QStringLiteral("name"), newItem.name));
		}
		if (oldItem.subscription != newItem.subscription) {
			record.append(createSqlField(QStringLiteral("subscription"), newItem.subscription));
		}
		if (oldItem.groups != newItem.groups) {
			updateGroups(oldItem, newItem);
		}

		if (!record.isEmpty()) {
			updateItemByRecord(oldItem.jid, record);
		}
	});
}

QFuture<QVector<RosterItem>> RosterDb::fetchItems()
{
	return run([this]() {
		auto query = createQuery();
		execQuery(query, QStringLiteral("SELECT * FROM roster"));

		QVector<RosterItem> items;
		parseItemsFromQuery(query, items);

		for (auto &item : items) {
			auto lastMessage = MessageDb::instance()->_fetchLastMessage(item.accountJid, item.jid);
			item.lastMessageDateTime = lastMessage.timestamp;
			item.lastMessage = lastMessage.previewText();
			item.lastMessageDeliveryState = lastMessage.deliveryState;
			item.lastMessageSenderId = lastMessage.senderId;
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
		{ QStringLiteral("jid"), jid }
	};

	execQuery(
		query,
		driver.sqlStatement(
			QSqlDriver::UpdateStatement,
			QStringLiteral(DB_TABLE_ROSTER),
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
			QStringLiteral(R"(
				SELECT name
				FROM rosterGroups
				WHERE accountJid = :accountJid AND chatJid = :jid
				LIMIT 1
			)"),
			{
				{ u":accountJid", item.accountJid },
				{ u":jid", item.jid },
			}
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
			QStringLiteral("INSERT OR IGNORE INTO " DB_TABLE_ROSTER_GROUPS
			"(accountJid, chatJid, name) VALUES(:accountJid, :chatJid, :name)"),
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
					QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
					"WHERE accountJid = :accountJid AND chatJid = :chatJid AND name = :name"),
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
					QStringLiteral("INSERT or IGNORE INTO " DB_TABLE_ROSTER_GROUPS " "
					"(accountJid, chatJid, name) "
					"VALUES (:accountJid, :chatJid, :name)"),
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
		QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
		"WHERE accountJid = :accountJid"),
		std::vector<QueryBindValue> { { u":accountJid", accountJid } }
	);
}

void RosterDb::removeGroups(const QString &accountJid, const QString &jid)
{
	auto query = createQuery();

	execQuery(
		query,
		QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_GROUPS " "
		"WHERE accountJid = :accountJid AND chatJid = :chatJid"),
		{ { u":accountJid", accountJid },
		  { u":chatJid", jid } }
	);
}
