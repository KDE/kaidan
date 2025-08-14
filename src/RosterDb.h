// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "DatabaseComponent.h"

struct RosterItem;

class RosterDb : public DatabaseComponent
{
    Q_OBJECT

public:
    explicit RosterDb(QObject *parent = nullptr);
    ~RosterDb() override;

    static RosterDb *instance();

    QFuture<QList<RosterItem>> fetchItems();

    QFuture<void> addItem(RosterItem item);
    Q_SIGNAL void itemAdded(const RosterItem &item);

    QFuture<void> updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    Q_SIGNAL void itemUpdated(const RosterItem &item);

    QFuture<void> replaceItems(const QString &accountJid, const QList<RosterItem> &items);
    Q_SIGNAL void itemsReplaced(const QString &accountJid);

    QFuture<void> removeItem(const QString &accountJid, const QString &jid);
    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);

    QFuture<void> removeItems(const QString &accountJid);
    Q_SIGNAL void itemsRemoved(const QString &accountJid);

private:
    QList<RosterItem> _fetchItems();
    QList<RosterItem> fetchBasicItems();

    void fetchGroups(RosterItem &item);
    void addGroups(const QString &accountJid, const QString &jid, const QList<QString> &groups);
    void updateGroups(const RosterItem &oldItem, const RosterItem &newItem);
    void removeGroups(const QString &accountJid);
    void removeGroups(const QString &accountJid, const QString &jid);

    void fetchLastMessage(RosterItem &item);
    void fetchLastMessage(RosterItem &item, QList<RosterItem> allItems);

    void fetchUnreadMessageCount(RosterItem &item);

    void _addItem(RosterItem item);
    void _updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    void _replaceItem(const RosterItem &oldItem, const RosterItem &newItem);
    void _removeItem(const QString &accountJid, const QString &jid);

    static QList<RosterItem> parseItemsFromQuery(QSqlQuery &query);
    static RosterItem parseItemFromQuery(QSqlQuery &query);

    void updateItemByRecord(const QString &accountJid, const QString &jid, const QSqlRecord &record);

    static RosterDb *s_instance;
};
