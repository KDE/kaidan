// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterDb.h"

// Qt
#include <QSqlQuery>
// Kaidan
#include "Globals.h"
#include "SqlUtils.h"

Q_DECLARE_METATYPE(QXmppRosterIq::Item)

using namespace SqlUtils;

RosterDb *RosterDb::s_instance = nullptr;

RosterDb::RosterDb(QObject *parent)
    : DatabaseComponent(parent)
{
    Q_ASSERT(!RosterDb::s_instance);
    s_instance = this;

    // Required so the item-carrying signals can be delivered across the database worker thread.
    qRegisterMetaType<QXmppRosterIq::Item>();
}

RosterDb::~RosterDb()
{
    s_instance = nullptr;
}

RosterDb *RosterDb::instance()
{
    return s_instance;
}

QXmppTask<RosterDb::RosterCache> RosterDb::load(const QString &accountJid)
{
    return runTask([this, accountJid]() {
        return _load(accountJid);
    });
}

QXmppTask<void> RosterDb::replaceAll(const QString &accountJid, const QString &version, const std::vector<Item> &items)
{
    return runTask([this, accountJid, version, items]() {
        transaction();

        _removeAll(accountJid);
        for (const auto &item : items) {
            _upsertItem(accountJid, item);
        }
        _setVersion(accountJid, version);

        commit();

        Q_EMIT replaced(accountJid);
    });
}

QXmppTask<void> RosterDb::upsertItem(const QString &accountJid, const QString &version, const Item &item)
{
    return runTask([this, accountJid, version, item]() {
        transaction();

        _upsertItem(accountJid, item);
        _setVersion(accountJid, version);

        commit();

        Q_EMIT itemUpserted(accountJid, item);
    });
}

QXmppTask<void> RosterDb::removeItem(const QString &accountJid, const QString &version, const QString &bareJid)
{
    return runTask([this, accountJid, version, bareJid]() {
        transaction();

        _removeItem(accountJid, bareJid);
        _setVersion(accountJid, version);

        commit();

        Q_EMIT itemRemoved(accountJid, bareJid);
    });
}

QXmppTask<void> RosterDb::clear(const QString &accountJid)
{
    return runTask([this, accountJid]() {
        transaction();

        _removeAll(accountJid);

        auto query = createQuery();
        execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_VERSIONS " WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});

        commit();

        Q_EMIT cleared(accountJid);
    });
}

RosterDb::RosterCache RosterDb::_load(const QString &accountJid)
{
    return RosterCache{_version(accountJid), _items(accountJid)};
}

std::vector<RosterDb::Item> RosterDb::_items(const QString &accountJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("SELECT jid, name, subscription, groupChatParticipantId, subscriptionStatus, approved, isMixChannel "
                             "FROM " DB_TABLE_ROSTER " WHERE accountJid = :accountJid"),
              {{u":accountJid", accountJid}});

    std::vector<Item> items;
    while (query.next()) {
        const auto jid = query.value(0).toString();

        Item item;
        item.setBareJid(jid);
        item.setName(query.value(1).toString());
        item.setSubscriptionType(static_cast<Item::SubscriptionType>(query.value(2).toInt()));
        item.setMixParticipantId(query.value(3).toString());
        item.setSubscriptionStatus(query.value(4).toString());
        item.setIsApproved(query.value(5).toBool());
        item.setIsMixChannel(query.value(6).toBool());
        item.setGroups(_groups(accountJid, jid));

        items.push_back(std::move(item));
    }

    return items;
}

void RosterDb::_upsertItem(const QString &accountJid, const Item &item)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("INSERT INTO " DB_TABLE_ROSTER " "
                             "(accountJid, jid, name, subscription, groupChatParticipantId, subscriptionStatus, approved, isMixChannel) "
                             "VALUES (:accountJid, :jid, :name, :subscription, :groupChatParticipantId, :subscriptionStatus, :approved, :isMixChannel) "
                             "ON CONFLICT(accountJid, jid) DO UPDATE SET "
                             "name = excluded.name, subscription = excluded.subscription, "
                             "groupChatParticipantId = excluded.groupChatParticipantId, "
                             "subscriptionStatus = excluded.subscriptionStatus, approved = excluded.approved, "
                             "isMixChannel = excluded.isMixChannel"),
              {
                  {u":accountJid", accountJid},
                  {u":jid", item.bareJid()},
                  {u":name", item.name()},
                  {u":subscription", static_cast<int>(item.subscriptionType())},
                  {u":groupChatParticipantId", item.mixParticipantId()},
                  {u":subscriptionStatus", item.subscriptionStatus()},
                  {u":approved", item.isApproved()},
                  {u":isMixChannel", item.isMixChannel()},
              });

    _writeGroups(accountJid, item.bareJid(), item.groups());
}

void RosterDb::_removeItem(const QString &accountJid, const QString &bareJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("DELETE FROM " DB_TABLE_ROSTER " WHERE accountJid = :accountJid AND jid = :jid"),
              {{u":accountJid", accountJid}, {u":jid", bareJid}});

    _removeGroups(accountJid, bareJid);
}

void RosterDb::_removeAll(const QString &accountJid)
{
    auto query = createQuery();
    execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_ROSTER " WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});
    execQuery(query, QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_GROUPS " WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});
}

QSet<QString> RosterDb::_groups(const QString &accountJid, const QString &jid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("SELECT name FROM " DB_TABLE_ROSTER_GROUPS " WHERE accountJid = :accountJid AND chatJid = :chatJid"),
              {{u":accountJid", accountJid}, {u":chatJid", jid}});

    QSet<QString> groups;
    while (query.next()) {
        groups.insert(query.value(0).toString());
    }
    return groups;
}

void RosterDb::_writeGroups(const QString &accountJid, const QString &jid, const QSet<QString> &groups)
{
    // The roster push carries the authoritative group set; replace whatever was stored before.
    _removeGroups(accountJid, jid);

    auto query = createQuery();
    for (const auto &group : groups) {
        execQuery(query,
                  QStringLiteral("INSERT OR IGNORE INTO " DB_TABLE_ROSTER_GROUPS " (accountJid, chatJid, name) "
                                 "VALUES (:accountJid, :chatJid, :name)"),
                  {{u":accountJid", accountJid}, {u":chatJid", jid}, {u":name", group}});
    }
}

void RosterDb::_removeGroups(const QString &accountJid, const QString &jid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_GROUPS " WHERE accountJid = :accountJid AND chatJid = :chatJid"),
              {{u":accountJid", accountJid}, {u":chatJid", jid}});
}

QString RosterDb::_version(const QString &accountJid)
{
    auto query = createQuery();
    execQuery(query, QStringLiteral("SELECT version FROM " DB_TABLE_ROSTER_VERSIONS " WHERE accountJid = :accountJid"), {{u":accountJid", accountJid}});

    if (query.next()) {
        return query.value(0).toString();
    }
    return {};
}

void RosterDb::_setVersion(const QString &accountJid, const QString &version)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("INSERT INTO " DB_TABLE_ROSTER_VERSIONS " (accountJid, version) VALUES (:accountJid, :version) "
                             "ON CONFLICT(accountJid) DO UPDATE SET version = excluded.version"),
              {{u":accountJid", accountJid}, {u":version", version}});
}

#include "moc_RosterDb.cpp"
