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
#include "Encryption.h"

struct RosterItem;

class RosterDb : public DatabaseComponent
{
    Q_OBJECT

public:
    explicit RosterDb(QObject *parent = nullptr);
    ~RosterDb() override;

    static RosterDb *instance();

    QFuture<QList<RosterItem>> fetchItems();

    Q_SIGNAL void itemAdded(const RosterItem &item);

    QFuture<void> updateItem(const QString &accountJid, const QString &jid, const std::function<void(RosterItem &)> &updateItem);
    Q_SIGNAL void itemUpdated(const RosterItem &item);

    Q_SIGNAL void itemsReplaced(const QString &accountJid);

    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);

    QFuture<void> removeItems(const QString &accountJid);
    Q_SIGNAL void itemsRemoved(const QString &accountJid);

    // Implementation of QXmppRosterStorage, scoped to a single account, used by RosterStorage.
    QXmppTask<QXmppRosterStorage::RosterCache> fetchRosterCache(const QString &accountJid);
    QXmppTask<void> replaceRoster(const QString &accountJid, const QString &version, const QList<RosterItem> &items);
    QXmppTask<void> upsertRosterItem(const QString &accountJid, const QString &version, RosterItem item);
    QXmppTask<void> removeRosterItem(const QString &accountJid, const QString &version, const QString &jid);
    QXmppTask<void> clearRoster(const QString &accountJid);

private:
    template<typename Functor>
    auto runTask(Functor function)
    {
        return runAsyncTask(this, dbWorker(), function);
    }

    QList<RosterItem> _fetchItems();
    QList<RosterItem> fetchBasicItems();
    QList<RosterItem> fetchWireItems(const QString &accountJid);

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
    void _upsertItem(const QString &accountJid, RosterItem item);
    void _replaceItems(const QString &accountJid, const QList<RosterItem> &items);
    void _removeItem(const QString &accountJid, const QString &jid);
    void _removeItems(const QString &accountJid);

    QString _fetchRosterVersion(const QString &accountJid);
    void _updateRosterVersion(const QString &accountJid, const QString &version);

    static QList<RosterItem> parseItemsFromQuery(QSqlQuery &query);
    static RosterItem parseItemFromQuery(QSqlQuery &query);

    void updateItemByRecord(const QString &accountJid, const QString &jid, const QSqlRecord &record);

    static RosterDb *s_instance;
};
