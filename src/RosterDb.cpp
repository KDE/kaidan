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
	if (oldItem.notificationsMuted != newItem.notificationsMuted)
		rec.append(createSqlField("notificationsMuted", newItem.notificationsMuted));
	if (oldItem.draftMessageId != newItem.draftMessageId)
		rec.append(createSqlField("draftMessageId", newItem.draftMessageId));
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
			query.addBindValue(QString()); // draftMessageId
			execQuery(query);
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

		// update loaded item
		if (!items.isEmpty()) {
			RosterItem item = items.first();
			updateItem(item);

			// replace old item with updated one, if item has changed
			if (items.first() != item) {
				// create an SQL record with only the differences
				QSqlRecord rec = createUpdateRecord(items.first(), item);

				if (rec.isEmpty())
					return;

				updateItemByRecord(jid, rec);
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

QFuture<void> RosterDb::removeItems(const QString &, const QString &)
{
	return run([this]() {
		auto query = createQuery();
		execQuery(query, "DELETE FROM roster");
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
			item.lastExchanged = lastMessage.stamp;
			item.lastMessage = lastMessage.previewText();
		}

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
