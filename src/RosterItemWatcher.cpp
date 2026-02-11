// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterItemWatcher.h"

// Kaidan
#include "RosterModel.h"

RosterItemNotifier &RosterItemNotifier::instance()
{
    static RosterItemNotifier notifier;
    return notifier;
}

void RosterItemNotifier::notifyWatchers(const QString &accountJid, const QString &jid, const std::optional<RosterItem> &item)
{
    std::ranges::for_each(m_itemWatchers, [&](RosterItemWatcher *rosterItemWatcher) {
        if (rosterItemWatcher->accountJid() == accountJid && rosterItemWatcher->jid() == jid) {
            rosterItemWatcher->notify(item);
        }
    });
}

void RosterItemNotifier::registerItemWatcher(RosterItemWatcher *watcher)
{
    auto itr = std::ranges::find_if(m_itemWatchers, [watcher](RosterItemWatcher *rosterItemWatcher) {
        return rosterItemWatcher == watcher;
    });

    if (itr == m_itemWatchers.end()) {
        m_itemWatchers.append(watcher);
    }
}

void RosterItemNotifier::unregisterItemWatcher(RosterItemWatcher *watcher)
{
    auto itr = std::ranges::find_if(m_itemWatchers, [watcher](RosterItemWatcher *rosterItemWatcher) {
        return rosterItemWatcher == watcher;
    });

    if (itr != m_itemWatchers.end()) {
        m_itemWatchers.erase(itr);
    }
}

RosterItemWatcher::RosterItemWatcher(QObject *parent)
    : QObject(parent)
{
}

RosterItemWatcher::~RosterItemWatcher()
{
    unregister();
}

const QString &RosterItemWatcher::accountJid() const
{
    return m_accountJid;
}

void RosterItemWatcher::setAccountJid(const QString &accountJid)
{
    if (accountJid != m_accountJid) {
        unregister();
        m_accountJid = accountJid;
        RosterItemNotifier::instance().registerItemWatcher(this);
        Q_EMIT accountJidChanged();
        notify(RosterModel::instance()->item(m_accountJid, m_jid));
    }
}

const QString &RosterItemWatcher::jid() const
{
    return m_jid;
}

void RosterItemWatcher::setJid(const QString &jid)
{
    if (jid != m_jid) {
        unregister();
        m_jid = jid;
        RosterItemNotifier::instance().registerItemWatcher(this);
        Q_EMIT jidChanged();
        notify(RosterModel::instance()->item(m_accountJid, m_jid));
    }
}

const RosterItem &RosterItemWatcher::item() const
{
    return m_item;
}

void RosterItemWatcher::unregister()
{
    if (!m_accountJid.isNull() && !m_jid.isNull()) {
        RosterItemNotifier::instance().unregisterItemWatcher(this);
    }
}

void RosterItemWatcher::notify(const std::optional<RosterItem> &item)
{
    if (item) {
        m_item = *item;
    } else {
        m_item = {};
    }

    Q_EMIT itemChanged();
}

#include "moc_RosterItemWatcher.cpp"
