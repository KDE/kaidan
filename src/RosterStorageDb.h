// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppRosterStorage.h>
// Kaidan
#include "DatabaseComponent.h"
#include "FutureUtils.h"

class AccountSettings;

/**
 * Persistent QXmppRosterStorage backend for QXmppRosterManager (RFC 6121 §2.6
 * roster versioning cache).
 *
 * The data is kept in its own SQL tables that are managed exclusively by this
 * class. This duplicates the roster data Kaidan already stores in RosterDb, but
 * keeps the cache independent for now.
 */
class RosterStorageDb : public DatabaseComponent, public QXmppRosterStorage
{
public:
    explicit RosterStorageDb(AccountSettings *accountSettings, QObject *parent = nullptr);

    QXmppTask<RosterCache> load() override;
    QXmppTask<void> replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items) override;
    QXmppTask<void> upsertItem(const QString &version, const QXmppRosterIq::Item &item) override;
    QXmppTask<void> removeItem(const QString &version, const QString &bareJid) override;
    QXmppTask<void> clear() override;

private:
    template<typename Functor>
    auto runTask(Functor function)
    {
        return runAsyncTask(this, dbWorker(), function);
    }

    void _upsertItem(const QXmppRosterIq::Item &item);
    void _removeItem(const QString &bareJid);
    void _setVersion(const QString &version);

    QString accountJid() const;

    AccountSettings *const m_accountSettings;
};
