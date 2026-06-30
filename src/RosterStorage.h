// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppRosterStorage.h>

class AccountSettings;

/**
 * Persists the roster (RFC 6121 §2.6 roster versioning) in Kaidan's roster table.
 *
 * This is a thin per-account adapter: every operation forwards to the RosterDb singleton with the
 * account's JID. An upsert only overrides the roster-wire columns (name, subscription, groups) so
 * that the remaining conversation data of an existing item is preserved.
 */
class RosterStorage : public QXmppRosterStorage
{
public:
    explicit RosterStorage(AccountSettings *accountSettings);

    QXmppTask<RosterCache> load() override;
    QXmppTask<void> replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items) override;
    QXmppTask<void> upsertItem(const QString &version, const QXmppRosterIq::Item &item) override;
    QXmppTask<void> removeItem(const QString &version, const QString &bareJid) override;
    QXmppTask<void> clear() override;

private:
    AccountSettings *const m_accountSettings;
};
