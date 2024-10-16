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

	QFuture<QVector<RosterItem>> fetchItems();

	QFuture<void> addItem(RosterItem item);
	Q_SIGNAL void itemAdded(const RosterItem &item);

	QFuture<void> updateItem(const QString &jid, const std::function<void (RosterItem &)> &updateItem);
	Q_SIGNAL void itemUpdated(const RosterItem &item);

	QFuture<void> replaceItems(const QHash<QString, RosterItem> &items);
	Q_SIGNAL void itemsReplaced();

	QFuture<void> removeItem(const QString &accountJid, const QString &jid);
	Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);

	QFuture<void> removeItems(const QString &accountJid);
	Q_SIGNAL void itemsRemoved(const QString &accountJid);

private:
	QVector<RosterItem> _fetchItems();

	void fetchGroups(QVector<RosterItem> &items);
	void addGroups(const QString &accountJid, const QString &jid, const QVector<QString> &groups);
	void updateGroups(const RosterItem &oldItem, const RosterItem &newItem);
	void removeGroups(const QString &accountJid);
	void removeGroups(const QString &accountJid, const QString &jid);

	void fetchLastMessages(QVector<RosterItem> &items);
	void fetchLastMessage(RosterItem &item, const QVector<RosterItem> &items);

	void _addItem(const RosterItem &item);
	void _updateItem(const QString &jid, const std::function<void (RosterItem &)> &updateItem);
	void _replaceItem(const RosterItem &oldItem, const RosterItem &newItem);
	void _removeItem(const QString &accountJid, const QString &jid);

	void updateItemByRecord(const QString &jid, const QSqlRecord &record);

	static RosterDb *s_instance;
};
