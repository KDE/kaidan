// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DatabaseComponent.h"

struct RosterItem;

class RosterDb : public DatabaseComponent
{
	Q_OBJECT

public:
	RosterDb(Database *db, QObject *parent = nullptr);
	~RosterDb();

	static RosterDb *instance();

	static void parseItemsFromQuery(QSqlQuery &query, QVector<RosterItem> &items);

	/**
	 * Creates an @c QSqlRecord for updating an old item to a new item.
	 *
	 * @param oldMsg Full item as it is currently saved
	 * @param newMsg Full item as it should be after the update query ran.
	 */
	static QSqlRecord createUpdateRecord(const RosterItem &oldItem,
	                                     const RosterItem &newItem);

	QFuture<void> addItem(RosterItem item);
	Q_SIGNAL void itemAdded(const RosterItem &item);
	QFuture<void> updateItem(const QString &jid,
	                const std::function<void (RosterItem &)> &updateItem);
	QFuture<void> replaceItems(const QHash<QString, RosterItem> &items);

	QFuture<void> removeItem(const QString &accountJid, const QString &jid);
	QFuture<void> removeItems(const QString &accountJid);
	QFuture<void> replaceItem(const RosterItem &oldItem, const RosterItem &newItem);
	QFuture<QVector<RosterItem>> fetchItems();


private:
	void updateItemByRecord(const QString &jid, const QSqlRecord &record);

	QVector<RosterItem> _fetchItems();

	void fetchGroups(QVector<RosterItem> &items);
	void addGroups(const QString &accountJid, const QString &jid, const QVector<QString> &groups);
	void updateGroups(const RosterItem &oldItem, const RosterItem &newItem);
	void removeGroups(const QString &accountJid);
	void removeGroups(const QString &accountJid, const QString &jid);

	void fetchLastMessages(QVector<RosterItem> &items);
	void fetchLastMessage(RosterItem &item, const QVector<RosterItem> &items);

	void _addItem(const RosterItem &item);

	static RosterDb *s_instance;
};
