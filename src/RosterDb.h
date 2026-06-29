// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <vector>
// QXmpp
#include <QXmppRosterStorage.h>
#include <QXmppTask.h>
// Kaidan
#include "DatabaseComponent.h"

class QSqlQuery;

/**
 * Persists the pure XMPP roster (RFC 6121) together with the server-provided roster version.
 *
 * RosterDb is a singleton owning the "roster", "rosterGroups" and "rosterVersions" tables. It
 * operates on wire-level QXmppRosterIq::Item objects and returns QXmppTask so a thin per-account
 * QXmppRosterStorage adapter can forward to it 1:1. Writes emit Qt signals so that UI-side
 * components can refresh the affected rows.
 */
class RosterDb : public DatabaseComponent
{
    Q_OBJECT

public:
    using RosterCache = QXmppRosterStorage::RosterCache;
    using Item = QXmppRosterIq::Item;

    explicit RosterDb(QObject *parent = nullptr);
    ~RosterDb() override;

    static RosterDb *instance();

    /// Loads the persisted roster version and items of the given account.
    QXmppTask<RosterCache> load(const QString &accountJid);

    /// Replaces every persisted item of the account and stores the new version.
    QXmppTask<void> replaceAll(const QString &accountJid, const QString &version, const std::vector<Item> &items);

    /// Inserts or replaces a single item and stores the new version.
    QXmppTask<void> upsertItem(const QString &accountJid, const QString &version, const Item &item);

    /// Removes the item with the given bare JID (if present) and stores the new version.
    QXmppTask<void> removeItem(const QString &accountJid, const QString &version, const QString &bareJid);

    /// Wipes all persisted items and the version of the account.
    QXmppTask<void> clear(const QString &accountJid);

    Q_SIGNAL void itemUpserted(const QString &accountJid, const QXmppRosterIq::Item &item);
    Q_SIGNAL void itemRemoved(const QString &accountJid, const QString &jid);
    Q_SIGNAL void replaced(const QString &accountJid);
    Q_SIGNAL void cleared(const QString &accountJid);

private:
    template<typename Functor>
    auto runTask(Functor function)
    {
        return runAsyncTask(this, dbWorker(), function);
    }

    RosterCache _load(const QString &accountJid);
    std::vector<Item> _items(const QString &accountJid);

    void _upsertItem(const QString &accountJid, const Item &item);
    void _removeItem(const QString &accountJid, const QString &bareJid);
    void _removeAll(const QString &accountJid);

    QSet<QString> _groups(const QString &accountJid, const QString &jid);
    void _writeGroups(const QString &accountJid, const QString &jid, const QSet<QString> &groups);
    void _removeGroups(const QString &accountJid, const QString &jid);

    QString _version(const QString &accountJid);
    void _setVersion(const QString &accountJid, const QString &version);

    static RosterDb *s_instance;
};
