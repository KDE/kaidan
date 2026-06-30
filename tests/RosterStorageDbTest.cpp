// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: LGPL-2.1-or-later

// Qt
#include <QTest>
// QXmpp
#include <QXmppRosterIq.h>
// Kaidan
#include "Account.h"
#include "Database.h"
#include "RosterStorageDb.h"
#include "Test.h"
#include "TestUtils.h"

using Item = QXmppRosterIq::Item;

namespace
{
Item makeItem(const QString &jid,
              const QString &name = {},
              Item::SubscriptionType subscription = Item::Both,
              const QSet<QString> &groups = {},
              const QString &ask = {},
              bool approved = false)
{
    Item item;
    item.setBareJid(jid);
    item.setName(name);
    item.setSubscriptionType(subscription);
    item.setGroups(groups);
    item.setSubscriptionStatus(ask);
    item.setIsApproved(approved);
    return item;
}

std::optional<Item> findItem(const QXmppRosterStorage::RosterCache &cache, const QString &jid)
{
    for (const auto &item : cache.items) {
        if (item.bareJid() == jid) {
            return item;
        }
    }
    return std::nullopt;
}

void compareItem(const Item &actual, const Item &expected)
{
    QCOMPARE(actual.bareJid(), expected.bareJid());
    QCOMPARE(actual.name(), expected.name());
    QCOMPARE(actual.subscriptionType(), expected.subscriptionType());
    QCOMPARE(actual.subscriptionStatus(), expected.subscriptionStatus());
    QCOMPARE(actual.isApproved(), expected.isApproved());
    QCOMPARE(actual.isMixChannel(), expected.isMixChannel());
    QCOMPARE(actual.mixParticipantId(), expected.mixParticipantId());
    QCOMPARE(actual.groups(), expected.groups());
}
}

class RosterStorageDbTest : public Test
{
    Q_OBJECT

public:
    RosterStorageDbTest();

private:
    Q_SLOT void testLoadEmpty();
    Q_SLOT void testReplaceAll();
    Q_SLOT void testUpsertItem();
    Q_SLOT void testRemoveItem();
    Q_SLOT void testFieldRoundtrip();
    Q_SLOT void testClear();
    Q_SLOT void testAccountScoping();

    Database db;
    std::unique_ptr<RosterStorageDb> storage;
};

RosterStorageDbTest::RosterStorageDbTest()
{
    AccountSettings::Data settingsData;
    settingsData.jid = QStringLiteral("user@example.org");
    storage = std::make_unique<RosterStorageDb>(new AccountSettings(settingsData, this));
}

void RosterStorageDbTest::testLoadEmpty()
{
    wait(this, storage->clear());

    auto cache = wait(this, storage->load());
    QVERIFY(cache.version.isEmpty());
    QVERIFY(cache.items.empty());
}

void RosterStorageDbTest::testReplaceAll()
{
    wait(this, storage->clear());

    wait(this,
         storage->replaceAll(QStringLiteral("v1"),
                             {
                                 makeItem(QStringLiteral("alice@example.org"), QStringLiteral("Alice")),
                                 makeItem(QStringLiteral("bob@example.com"), QStringLiteral("Bob")),
                             }));

    {
        auto cache = wait(this, storage->load());
        QCOMPARE(cache.version, QStringLiteral("v1"));
        QCOMPARE(cache.items.size(), std::size_t(2));
        QVERIFY(findItem(cache, QStringLiteral("alice@example.org")).has_value());
        QVERIFY(findItem(cache, QStringLiteral("bob@example.com")).has_value());
    }

    // A second replaceAll must not merge with the previous items.
    wait(this,
         storage->replaceAll(QStringLiteral("v2"),
                             {
                                 makeItem(QStringLiteral("carol@example.net"), QStringLiteral("Carol")),
                             }));

    {
        auto cache = wait(this, storage->load());
        QCOMPARE(cache.version, QStringLiteral("v2"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        QVERIFY(findItem(cache, QStringLiteral("carol@example.net")).has_value());
        QVERIFY(!findItem(cache, QStringLiteral("alice@example.org")).has_value());
    }
}

void RosterStorageDbTest::testUpsertItem()
{
    wait(this, storage->clear());

    wait(this, storage->upsertItem(QStringLiteral("v1"), makeItem(QStringLiteral("alice@example.org"), QStringLiteral("Alice"))));

    {
        auto cache = wait(this, storage->load());
        QCOMPARE(cache.version, QStringLiteral("v1"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        QCOMPARE(findItem(cache, QStringLiteral("alice@example.org"))->name(), QStringLiteral("Alice"));
    }

    // Upserting the same JID replaces the item in place and updates the version.
    wait(this, storage->upsertItem(QStringLiteral("v2"), makeItem(QStringLiteral("alice@example.org"), QStringLiteral("Alice in Wonderland"), Item::To)));

    {
        auto cache = wait(this, storage->load());
        QCOMPARE(cache.version, QStringLiteral("v2"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        auto item = findItem(cache, QStringLiteral("alice@example.org"));
        QCOMPARE(item->name(), QStringLiteral("Alice in Wonderland"));
        QCOMPARE(item->subscriptionType(), Item::To);
    }
}

void RosterStorageDbTest::testRemoveItem()
{
    wait(this, storage->clear());

    wait(this,
         storage->replaceAll(QStringLiteral("v1"),
                             {
                                 makeItem(QStringLiteral("alice@example.org")),
                                 makeItem(QStringLiteral("bob@example.com")),
                             }));

    wait(this, storage->removeItem(QStringLiteral("v2"), QStringLiteral("alice@example.org")));

    auto cache = wait(this, storage->load());
    // The version is updated even though only one item was removed.
    QCOMPARE(cache.version, QStringLiteral("v2"));
    QCOMPARE(cache.items.size(), std::size_t(1));
    QVERIFY(!findItem(cache, QStringLiteral("alice@example.org")).has_value());
    QVERIFY(findItem(cache, QStringLiteral("bob@example.com")).has_value());
}

void RosterStorageDbTest::testFieldRoundtrip()
{
    wait(this, storage->clear());

    auto item = makeItem(QStringLiteral("alice@example.org"),
                         QStringLiteral("Alice"),
                         Item::From,
                         {QStringLiteral("Friends"), QStringLiteral("Family")},
                         QStringLiteral("subscribe"),
                         true);
    item.setIsMixChannel(true);
    item.setMixParticipantId(QStringLiteral("123456"));

    wait(this, storage->upsertItem(QStringLiteral("v1"), item));

    auto cache = wait(this, storage->load());
    QCOMPARE(cache.items.size(), std::size_t(1));
    compareItem(*findItem(cache, QStringLiteral("alice@example.org")), item);
}

void RosterStorageDbTest::testClear()
{
    wait(this,
         storage->replaceAll(QStringLiteral("v1"),
                             {
                                 makeItem(QStringLiteral("alice@example.org")),
                             }));

    wait(this, storage->clear());

    auto cache = wait(this, storage->load());
    QVERIFY(cache.version.isEmpty());
    QVERIFY(cache.items.empty());
}

void RosterStorageDbTest::testAccountScoping()
{
    wait(this, storage->clear());

    AccountSettings::Data otherData;
    otherData.jid = QStringLiteral("other@example.org");
    RosterStorageDb otherStorage(new AccountSettings(otherData, this));
    wait(this, otherStorage.clear());

    wait(this, storage->upsertItem(QStringLiteral("v1"), makeItem(QStringLiteral("alice@example.org"))));
    wait(this, otherStorage.upsertItem(QStringLiteral("other-v1"), makeItem(QStringLiteral("bob@example.com"))));

    // Each storage only sees its own account's data.
    {
        auto cache = wait(this, storage->load());
        QCOMPARE(cache.version, QStringLiteral("v1"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        QVERIFY(findItem(cache, QStringLiteral("alice@example.org")).has_value());
    }

    // Clearing one account does not affect the other.
    wait(this, storage->clear());

    {
        auto cache = wait(this, otherStorage.load());
        QCOMPARE(cache.version, QStringLiteral("other-v1"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        QVERIFY(findItem(cache, QStringLiteral("bob@example.com")).has_value());
    }

    wait(this, otherStorage.clear());
}

QTEST_GUILESS_MAIN(RosterStorageDbTest)
#include "RosterStorageDbTest.moc"
