// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "DatabaseComponent.h"

struct RosterItem;

class ChatDb : public DatabaseComponent
{
    Q_OBJECT

public:
    explicit ChatDb(QObject *parent = nullptr);
    ~ChatDb() override;

    static ChatDb *instance();

    QFuture<QList<RosterItem>> fetchItems();

    QFuture<void> addItem(RosterItem item);
    Q_SIGNAL void itemAdded(const RosterItem &item);

    QFuture<void> updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    Q_SIGNAL void itemUpdated(const RosterItem &item);

    // Re-reads the fused item and re-emits itemUpdated (e.g. after RosterDb changed its roster data).
    QFuture<void> refreshItem(const QString &accountJid, const QString &jid);

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

    void fetchLastMessage(RosterItem &item);
    void fetchLastMessage(RosterItem &item, const QList<RosterItem> &allItems);

    void fetchUnreadMessageCount(RosterItem &item);
    void fetchMarkedMessageCount(RosterItem &item);

    void _addItem(RosterItem item);
    void _updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    void _refreshItem(const QString &accountJid, const QString &jid);
    void _replaceItem(const RosterItem &oldItem, const RosterItem &newItem);
    void _removeItem(const QString &accountJid, const QString &jid);

    static QList<RosterItem> parseItemsFromQuery(QSqlQuery &query);
    static RosterItem parseItemFromQuery(QSqlQuery &query);

    void updateItemByRecord(const QString &table, const QString &accountJid, const QString &jid, const QSqlRecord &record);

    static ChatDb *s_instance;
};
