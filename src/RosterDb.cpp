/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2021 Kaidan developers and contributors
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
	int idxUnreadMessages = rec.indexOf("unreadMessages");

	while (query.next()) {
		RosterItem item;
		item.setJid(query.value(idxJid).toString());
		item.setName(query.value(idxName).toString());
		item.setUnreadMessages(query.value(idxUnreadMessages).toInt());

		items << std::move(item);
	}
}

QSqlRecord RosterDb::createUpdateRecord(const RosterItem &oldItem, const RosterItem &newItem)
{
	QSqlRecord rec;
	if (oldItem.jid() != newItem.jid())
		rec.append(createSqlField("jid", newItem.jid()));
	if (oldItem.name() != newItem.name())
		rec.append(createSqlField("name", newItem.name()));
	if (oldItem.unreadMessages() != newItem.unreadMessages())
		rec.append(createSqlField(
			"unreadMessages",
			newItem.unreadMessages()
		));
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
			query.addBindValue(item.jid());
			query.addBindValue(item.name());
			query.addBindValue(QLatin1String("")); // lastExchanged (NOT NULL)
			query.addBindValue(item.unreadMessages());
			query.addBindValue(QString()); // lastMessage
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
		execQuery(
				query,
				"SELECT * FROM Roster WHERE jid = ? LIMIT 1",
				QVector<QVariant>() << jid
		);

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
		execQuery(query, "SELECT * FROM Roster");

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
			if (newJids.remove(oldItem.jid())) {
				// item is also included in newJids -> update

				// name is (currently) the only attribute that is defined by the
				// XMPP roster and so could cause a change
				if (oldItem.name() != items[oldItem.jid()].name())
					setItemName(oldItem.jid(), items[oldItem.jid()].name());
			} else {
				// item is not included in newJids -> delete
				removeItems({}, oldItem.jid());
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
		execQuery(query, "DELETE FROM Roster");
	});
}

QFuture<void> RosterDb::setItemName(const QString &jid, const QString &name)
{
	return run([this, jid, name]() {
		auto query = createQuery();
		auto &driver = sqlDriver();

		QSqlRecord rec;
		rec.append(createSqlField("name", name));

		execQuery(
			query,
			driver.sqlStatement(
				QSqlDriver::UpdateStatement,
				DB_TABLE_ROSTER,
				rec,
				false
			) +
			simpleWhereStatement(&driver, "jid", jid)
		);
	});
}

QFuture<QVector<RosterItem>> RosterDb::fetchItems(const QString &accountId)
{
	return run([this, accountId]() {
		auto query = createQuery();
		execQuery(query, "SELECT * FROM Roster");

		QVector<RosterItem> items;
		parseItemsFromQuery(query, items);

		for (auto &item : items) {
			Message lastMessage = MessageDb::instance()->_fetchLastMessage(accountId, item.jid());
			item.setLastExchanged(lastMessage.stamp());
			item.setLastMessage(lastMessage.previewText());
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
