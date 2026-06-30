// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterStorageDb.h"

// Qt
#include <QJsonArray>
#include <QJsonDocument>
// Kaidan
#include "Account.h"
#include "Globals.h"
#include "SqlUtils.h"

using namespace SqlUtils;

namespace
{
QString serializeGroups(const QSet<QString> &groups)
{
    QJsonArray array;
    for (const auto &group : groups) {
        array.append(group);
    }
    return QString::fromUtf8(QJsonDocument(array).toJson(QJsonDocument::Compact));
}

QSet<QString> parseGroups(const QString &serialized)
{
    QSet<QString> groups;
    const auto array = QJsonDocument::fromJson(serialized.toUtf8()).array();
    for (const auto &value : array) {
        groups.insert(value.toString());
    }
    return groups;
}
}

RosterStorageDb::RosterStorageDb(AccountSettings *accountSettings, QObject *parent)
    : DatabaseComponent(parent)
    , m_accountSettings(accountSettings)
{
}

QXmppTask<QXmppRosterStorage::RosterCache> RosterStorageDb::load()
{
    return runTask([this]() -> RosterCache {
        RosterCache cache;

        auto versionQuery = createQuery();
        execQuery(versionQuery,
                  QStringLiteral("SELECT version FROM " DB_TABLE_ROSTER_STORAGE_VERSION " WHERE account = :account"),
                  {
                      {u":account", accountJid()},
                  });
        if (versionQuery.next()) {
            cache.version = versionQuery.value(0).toString();
        }

        auto itemsQuery = createQuery();
        execQuery(itemsQuery,
                  QStringLiteral("SELECT jid, name, subscription, ask, approved, isMixChannel, mixParticipantId, groups "
                                 "FROM " DB_TABLE_ROSTER_STORAGE " WHERE account = :account"),
                  {
                      {u":account", accountJid()},
                  });

        enum {
            Jid,
            Name,
            Subscription,
            Ask,
            Approved,
            IsMixChannel,
            MixParticipantId,
            Groups,
        };

        while (itemsQuery.next()) {
            QXmppRosterIq::Item item;
            item.setBareJid(itemsQuery.value(Jid).toString());
            item.setName(itemsQuery.value(Name).toString());
            item.setSubscriptionType(static_cast<QXmppRosterIq::Item::SubscriptionType>(itemsQuery.value(Subscription).toInt()));
            item.setSubscriptionStatus(itemsQuery.value(Ask).toString());
            item.setIsApproved(itemsQuery.value(Approved).toBool());
            item.setIsMixChannel(itemsQuery.value(IsMixChannel).toBool());
            item.setMixParticipantId(itemsQuery.value(MixParticipantId).toString());
            item.setGroups(parseGroups(itemsQuery.value(Groups).toString()));
            cache.items.push_back(std::move(item));
        }

        return cache;
    });
}

QXmppTask<void> RosterStorageDb::replaceAll(const QString &version, const std::vector<QXmppRosterIq::Item> &items)
{
    return runTask([this, version, items] {
        transaction();

        auto query = createQuery();
        execQuery(query,
                  QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_STORAGE " WHERE account = :account"),
                  {
                      {u":account", accountJid()},
                  });

        for (const auto &item : items) {
            _upsertItem(item);
        }

        _setVersion(version);

        commit();
    });
}

QXmppTask<void> RosterStorageDb::upsertItem(const QString &version, const QXmppRosterIq::Item &item)
{
    return runTask([this, version, item] {
        transaction();
        _upsertItem(item);
        _setVersion(version);
        commit();
    });
}

QXmppTask<void> RosterStorageDb::removeItem(const QString &version, const QString &bareJid)
{
    return runTask([this, version, bareJid] {
        transaction();
        _removeItem(bareJid);
        _setVersion(version);
        commit();
    });
}

QXmppTask<void> RosterStorageDb::clear()
{
    return runTask([this] {
        transaction();

        auto query = createQuery();
        execQuery(query,
                  QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_STORAGE " WHERE account = :account"),
                  {
                      {u":account", accountJid()},
                  });
        execQuery(query,
                  QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_STORAGE_VERSION " WHERE account = :account"),
                  {
                      {u":account", accountJid()},
                  });

        commit();
    });
}

void RosterStorageDb::_upsertItem(const QXmppRosterIq::Item &item)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
				INSERT OR REPLACE INTO )" DB_TABLE_ROSTER_STORAGE R"( (
					account,
					jid,
					name,
					subscription,
					ask,
					approved,
					isMixChannel,
					mixParticipantId,
					groups
				)
				VALUES (
					:account,
					:jid,
					:name,
					:subscription,
					:ask,
					:approved,
					:isMixChannel,
					:mixParticipantId,
					:groups
				)
			)"),
              {
                  {u":account", accountJid()},
                  {u":jid", item.bareJid()},
                  {u":name", item.name()},
                  {u":subscription", static_cast<int>(item.subscriptionType())},
                  {u":ask", item.subscriptionStatus()},
                  {u":approved", item.isApproved()},
                  {u":isMixChannel", item.isMixChannel()},
                  {u":mixParticipantId", item.mixParticipantId()},
                  {u":groups", serializeGroups(item.groups())},
              });
}

void RosterStorageDb::_removeItem(const QString &bareJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("DELETE FROM " DB_TABLE_ROSTER_STORAGE " WHERE account = :account AND jid = :jid"),
              {
                  {u":account", accountJid()},
                  {u":jid", bareJid},
              });
}

void RosterStorageDb::_setVersion(const QString &version)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral("INSERT OR REPLACE INTO " DB_TABLE_ROSTER_STORAGE_VERSION " (account, version) VALUES (:account, :version)"),
              {
                  {u":account", accountJid()},
                  {u":version", version},
              });
}

QString RosterStorageDb::accountJid() const
{
    return m_accountSettings->jid();
}
