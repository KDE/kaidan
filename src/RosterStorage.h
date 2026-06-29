// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QString>
// QXmpp
#include <QXmppRosterStorage.h>

/**
 * Per-account QXmppRosterStorage adapter that delegates to the RosterDb singleton.
 *
 * QXmppRosterManager owns one storage instance per account (installed via setStorage()) and scopes
 * all calls to that account. This adapter holds the account JID and forwards each operation to
 * RosterDb::instance(), which performs the SQL and returns the QXmppTask directly.
 */
class RosterStorage : public QXmppRosterStorage
{
public:
    explicit RosterStorage(QString accountJid);

    QXmppTask<RosterCache> load() override;
    QXmppTask<void> replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items) override;
    QXmppTask<void> upsertItem(const QString &version, const QXmppRosterIq::Item &item) override;
    QXmppTask<void> removeItem(const QString &version, const QString &bareJid) override;
    QXmppTask<void> clear() override;

private:
    const QString m_accountJid;
};
