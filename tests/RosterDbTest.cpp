// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QSignalSpy>
#include <QTest>
// QXmpp
#include <QXmppRosterIq.h>
// Kaidan
#include "Database.h"
#include "MainController.h"
#include "RosterDb.h"
#include "Test.h"
#include "TestUtils.h"

using Item = QXmppRosterIq::Item;

// Tests the SQL QXmppRosterStorage backend: the wire-level roster (RFC 6121) and its server version
// are persisted and round-tripped, and writes emit the refresh signals.
class RosterDbTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void initTestCase() override;
    Q_SLOT void cleanup();

    Q_SLOT void replaceAllRoundTrip();
    Q_SLOT void upsertInsertsAndUpdates();
    Q_SLOT void removeDeletesAndBumpsVersion();
    Q_SLOT void clearWipesEverything();
    Q_SLOT void signalsFire();

    Item contact(const QString &jid) const;
    std::optional<Item> find(const std::vector<Item> &items, const QString &jid) const;
    RosterDb::RosterCache load();

    std::unique_ptr<MainController> m_mainController;
    const QString m_accountJid = QStringLiteral("alice@example.org");
};

void RosterDbTest::initTestCase()
{
    Test::initTestCase();
    // Sets up the Database and RosterDb singletons (among others).
    m_mainController = std::make_unique<MainController>();
}

void RosterDbTest::cleanup()
{
    // Keep the tests independent of each other.
    wait(this, RosterDb::instance()->clear(m_accountJid));
}

Item RosterDbTest::contact(const QString &jid) const
{
    Item item;
    item.setBareJid(jid);
    return item;
}

std::optional<Item> RosterDbTest::find(const std::vector<Item> &items, const QString &jid) const
{
    for (const auto &item : items) {
        if (item.bareJid() == jid) {
            return item;
        }
    }
    return std::nullopt;
}

RosterDb::RosterCache RosterDbTest::load()
{
    return wait(this, RosterDb::instance()->load(m_accountJid));
}

// replaceAll stores the version and items (including the new wire fields and groups); load returns
// them verbatim.
void RosterDbTest::replaceAllRoundTrip()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    bob.setName(QStringLiteral("Bob"));
    bob.setSubscriptionType(Item::To);
    bob.setSubscriptionStatus(QStringLiteral("subscribe"));
    bob.setIsApproved(true);
    bob.setGroups({QStringLiteral("Friends"), QStringLiteral("Work")});

    auto channel = contact(QStringLiteral("team@mix.example.org"));
    channel.setSubscriptionType(Item::Both);
    channel.setIsMixChannel(true);
    channel.setMixParticipantId(QStringLiteral("123456"));

    wait(this, RosterDb::instance()->replaceAll(m_accountJid, QStringLiteral("ver-1"), {bob, channel}));

    const auto cache = load();
    QCOMPARE(cache.version, QStringLiteral("ver-1"));
    QCOMPARE(cache.items.size(), std::size_t(2));

    const auto fetchedBob = find(cache.items, bob.bareJid());
    QVERIFY(fetchedBob.has_value());
    QCOMPARE(fetchedBob->name(), QStringLiteral("Bob"));
    QCOMPARE(fetchedBob->subscriptionType(), Item::To);
    QCOMPARE(fetchedBob->subscriptionStatus(), QStringLiteral("subscribe"));
    QCOMPARE(fetchedBob->isApproved(), true);
    QCOMPARE(fetchedBob->isMixChannel(), false);
    QCOMPARE(fetchedBob->groups(), QSet<QString>({QStringLiteral("Friends"), QStringLiteral("Work")}));

    const auto fetchedChannel = find(cache.items, channel.bareJid());
    QVERIFY(fetchedChannel.has_value());
    QCOMPARE(fetchedChannel->subscriptionType(), Item::Both);
    QCOMPARE(fetchedChannel->isMixChannel(), true);
    QCOMPARE(fetchedChannel->mixParticipantId(), QStringLiteral("123456"));
    QVERIFY(fetchedChannel->groups().isEmpty());
}

// upsertItem inserts new items and updates existing ones, each time storing the new version.
void RosterDbTest::upsertInsertsAndUpdates()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    bob.setName(QStringLiteral("Bob"));
    bob.setSubscriptionType(Item::None);
    bob.setGroups({QStringLiteral("Friends")});
    wait(this, RosterDb::instance()->upsertItem(m_accountJid, QStringLiteral("ver-1"), bob));

    {
        const auto cache = load();
        QCOMPARE(cache.version, QStringLiteral("ver-1"));
        QCOMPARE(cache.items.size(), std::size_t(1));
        QCOMPARE(find(cache.items, bob.bareJid())->name(), QStringLiteral("Bob"));
    }

    // Update the existing item (rename, new subscription, changed groups) and bump the version.
    bob.setName(QStringLiteral("Bobby"));
    bob.setSubscriptionType(Item::Both);
    bob.setGroups({QStringLiteral("Work")});
    wait(this, RosterDb::instance()->upsertItem(m_accountJid, QStringLiteral("ver-2"), bob));

    {
        const auto cache = load();
        QCOMPARE(cache.version, QStringLiteral("ver-2"));
        // Still exactly one row: the existing item was updated, not duplicated.
        QCOMPARE(cache.items.size(), std::size_t(1));
        const auto fetched = find(cache.items, bob.bareJid());
        QCOMPARE(fetched->name(), QStringLiteral("Bobby"));
        QCOMPARE(fetched->subscriptionType(), Item::Both);
        QCOMPARE(fetched->groups(), QSet<QString>({QStringLiteral("Work")}));
    }
}

// removeItem deletes the row (and its groups) and stores the new version.
void RosterDbTest::removeDeletesAndBumpsVersion()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    bob.setGroups({QStringLiteral("Friends")});
    auto carol = contact(QStringLiteral("carol@example.com"));
    wait(this, RosterDb::instance()->replaceAll(m_accountJid, QStringLiteral("ver-1"), {bob, carol}));

    wait(this, RosterDb::instance()->removeItem(m_accountJid, QStringLiteral("ver-2"), bob.bareJid()));

    const auto cache = load();
    QCOMPARE(cache.version, QStringLiteral("ver-2"));
    QCOMPARE(cache.items.size(), std::size_t(1));
    QVERIFY(!find(cache.items, bob.bareJid()).has_value());
    QVERIFY(find(cache.items, carol.bareJid()).has_value());
}

// clear wipes the items and resets the version to empty.
void RosterDbTest::clearWipesEverything()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    bob.setGroups({QStringLiteral("Friends")});
    wait(this, RosterDb::instance()->replaceAll(m_accountJid, QStringLiteral("ver-1"), {bob}));

    wait(this, RosterDb::instance()->clear(m_accountJid));

    const auto cache = load();
    QVERIFY(cache.version.isEmpty());
    QVERIFY(cache.items.empty());
}

// Each mutating operation emits its refresh signal.
void RosterDbTest::signalsFire()
{
    auto *db = RosterDb::instance();

    QSignalSpy replacedSpy(db, &RosterDb::replaced);
    QSignalSpy upsertedSpy(db, &RosterDb::itemUpserted);
    QSignalSpy removedSpy(db, &RosterDb::itemRemoved);
    QSignalSpy clearedSpy(db, &RosterDb::cleared);

    auto bob = contact(QStringLiteral("bob@example.com"));

    wait(this, db->replaceAll(m_accountJid, QStringLiteral("ver-1"), {bob}));
    QCOMPARE(replacedSpy.count(), 1);
    QCOMPARE(replacedSpy.constFirst().at(0).toString(), m_accountJid);

    wait(this, db->upsertItem(m_accountJid, QStringLiteral("ver-2"), bob));
    QCOMPARE(upsertedSpy.count(), 1);
    QCOMPARE(upsertedSpy.constFirst().at(0).toString(), m_accountJid);

    wait(this, db->removeItem(m_accountJid, QStringLiteral("ver-3"), bob.bareJid()));
    QCOMPARE(removedSpy.count(), 1);
    QCOMPARE(removedSpy.constFirst().at(1).toString(), bob.bareJid());

    wait(this, db->clear(m_accountJid));
    QCOMPARE(clearedSpy.count(), 1);
}

QTEST_GUILESS_MAIN(RosterDbTest)
#include "RosterDbTest.moc"
