// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterStorage.h"

// Kaidan
#include "RosterDb.h"

RosterStorage::RosterStorage(QString accountJid)
    : m_accountJid(std::move(accountJid))
{
}

QXmppTask<QXmppRosterStorage::RosterCache> RosterStorage::load()
{
    return RosterDb::instance()->load(m_accountJid);
}

QXmppTask<void> RosterStorage::replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items)
{
    return RosterDb::instance()->replaceAll(m_accountJid, version, items);
}

QXmppTask<void> RosterStorage::upsertItem(const QString &version, const QXmppRosterIq::Item &item)
{
    return RosterDb::instance()->upsertItem(m_accountJid, version, item);
}

QXmppTask<void> RosterStorage::removeItem(const QString &version, const QString &bareJid)
{
    return RosterDb::instance()->removeItem(m_accountJid, version, bareJid);
}

QXmppTask<void> RosterStorage::clear()
{
    return RosterDb::instance()->clear(m_accountJid);
}
