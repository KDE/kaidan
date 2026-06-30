// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterStorage.h"

// Kaidan
#include "Account.h"
#include "RosterDb.h"
#include "RosterItem.h"

RosterStorage::RosterStorage(AccountSettings *accountSettings)
    : m_accountSettings(accountSettings)
{
}

QXmppTask<QXmppRosterStorage::RosterCache> RosterStorage::load()
{
    return RosterDb::instance()->fetchRosterCache(m_accountSettings->jid());
}

QXmppTask<void> RosterStorage::replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items)
{
    const auto accountJid = m_accountSettings->jid();
    const auto encryption = m_accountSettings->encryption();

    QList<RosterItem> rosterItems;
    rosterItems.reserve(items.size());

    for (const auto &item : items) {
        RosterItem rosterItem{accountJid, item};
        rosterItem.encryption = encryption;
        rosterItems.append(std::move(rosterItem));
    }

    return RosterDb::instance()->replaceRoster(accountJid, version, rosterItems);
}

QXmppTask<void> RosterStorage::upsertItem(const QString &version, const QXmppRosterIq::Item &item)
{
    const auto accountJid = m_accountSettings->jid();

    RosterItem rosterItem{accountJid, item};
    rosterItem.encryption = m_accountSettings->encryption();

    return RosterDb::instance()->upsertRosterItem(accountJid, version, std::move(rosterItem));
}

QXmppTask<void> RosterStorage::removeItem(const QString &version, const QString &bareJid)
{
    return RosterDb::instance()->removeRosterItem(m_accountSettings->jid(), version, bareJid);
}

QXmppTask<void> RosterStorage::clear()
{
    return RosterDb::instance()->clearRoster(m_accountSettings->jid());
}
