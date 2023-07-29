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

	QFuture<void> addItem(const RosterItem &item);
	QFuture<void> addItems(const QVector<RosterItem> &items);
	QFuture<void> updateItem(const QString &jid,
	                const std::function<void (RosterItem &)> &updateItem);
	QFuture<void> replaceItems(const QHash<QString, RosterItem> &items);

	/**
	 * Removes all roster items of an account or a specific roster item.
	 *
	 * @param accountJid JID of the account whose roster items are being removed
	 * @param jid JID of the roster item being removed (optional)
	 */
	QFuture<void> removeItems(const QString &accountJid, const QString &jid = {});
	QFuture<void> replaceItem(const RosterItem &oldItem, const RosterItem &newItem);
	QFuture<QVector<RosterItem>> fetchItems(const QString &accountId);


private:
	void updateItemByRecord(const QString &jid, const QSqlRecord &record);

	void fetchGroups(QVector<RosterItem> &items);
	void addGroups(const QString &accountJid, const QString &jid, const QVector<QString> &groups);
	void updateGroups(const RosterItem &oldItem, const RosterItem &newItem);
	void removeGroups(const QString &accountJid);
	void removeGroups(const QString &accountJid, const QString &jid);

	static RosterDb *s_instance;
};
