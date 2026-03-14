// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppRosterStorage.h>
#include <QXmppTask.h>
// Kaidan
#include "DatabaseComponent.h"
#include "RosterItem.h"

class RosterDb : public DatabaseComponent
{
    Q_OBJECT

public:
    explicit RosterDb(QObject *parent = nullptr);
    ~RosterDb() override;

    static RosterDb *instance();

    QFuture<QList<RosterItem>> fetchItems();
    QXmppTask<QXmppRosterStorage::RosterCache> fetchRosterCache(const QString &accountJid);

    QXmppTask<void> replaceItems(const QString &accountJid, const QString &version, const QList<RosterItem> &items);
    QFuture<void> replaceBookmarks(const QString &accountJid, const QList<RosterItem> &items);
    Q_SIGNAL void itemsReplaced(const QString &accountJid);

    QFuture<void> updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    QXmppTask<void> updateOrAddItem(const QString &accountJid, const QString &version, RosterItem item);
    QFuture<void> addBookmarks(const QString &accountJid, const QList<RosterItem> &items);
    Q_SIGNAL void itemAdded(const RosterItem &item);
    Q_SIGNAL void itemUpdated(const RosterItem &item);

    QXmppTask<void> removeItem(const QString &accountJid, const QString &version, const QString &jid);
    QFuture<void> removeBookmarks(const QString &accountJid, const QList<QString> &jids);
    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);

    QFuture<void> removeItems(const QString &accountJid);
    QFuture<void> removeBookmarks(const QString &accountJid);
    Q_SIGNAL void itemsRemoved(const QString &accountJid);

private:
    template<typename Functor>
    auto runTask(Functor function)
    {
        return runAsyncTask(this, dbWorker(), function);
    }

    QList<RosterItem> _fetchItems();
    QList<RosterItem> fetchBasicItems();
    QList<RosterItem> fetchWireItems(const QString &accountJid, RosterItem::Origin origin = RosterItem::Origin::Roster);

    void fetchGroups(RosterItem &item);
    void addGroups(const QString &accountJid, const QString &jid, const QList<QString> &groups);
    void updateGroups(const RosterItem &oldItem, const RosterItem &newItem);
    void removeGroups(const QString &accountJid);
    void removeGroups(const QString &accountJid, const QString &jid);

    void fetchLastMessage(RosterItem &item);
    void fetchLastMessage(RosterItem &item, const QList<RosterItem> &allItems);

    void fetchUnreadMessageCount(RosterItem &item);
    void fetchMarkedMessageCount(RosterItem &item);

    void _addItem(RosterItem item);
    void _updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    void _updateOrAddItem(const QString &accountJid, RosterItem item);
    void _replaceItems(const QString &accountJid, const QList<RosterItem> &items, RosterItem::Origin origin = RosterItem::Origin::Roster);
    void _removeItem(const QString &accountJid, const QString &jid);
    void _removeItems(const QString &accountJid);
    void _removeItems(const QString &accountJid, RosterItem::Origin origin);

    static QList<RosterItem> parseItemsFromQuery(QSqlQuery &query);
    static RosterItem parseItemFromQuery(QSqlQuery &query);

    void updateItemByRecord(const QString &accountJid, const QString &jid, const QSqlRecord &record);

    static RosterDb *s_instance;
};
