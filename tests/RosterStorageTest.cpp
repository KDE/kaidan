// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// std
#include <optional>
// Qt
#include <QTest>
// QXmpp
#include <QXmppRosterIq.h>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
#include "Database.h"
#include "Encryption.h"
#include "GroupChatUserDb.h"
#include "Keychain.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterItem.h"
#include "RosterStorage.h"
#include "Test.h"
#include "TestUtils.h"

static const auto accountJid = QStringLiteral("user@example.org");
static const auto alice = QStringLiteral("alice@example.org");
static const auto bob = QStringLiteral("bob@example.org");

class RosterStorageTest : public Test
{
    Q_OBJECT

public:
    RosterStorageTest();

private:
    Q_SLOT void initTestCase() override;
    Q_SLOT void init();

    Q_SLOT void testUpsertAddsItem();
    Q_SLOT void testUpsertPreservesConversationData();
    Q_SLOT void testReplaceAllPreservesConversationData();
    Q_SLOT void testRemoveItem();
    Q_SLOT void testClear();
    Q_SLOT void testLoadRoundTrip();
    Q_SLOT void testWireAttributesRoundTrip();

    static QXmppRosterIq::Item
    wireItem(const QString &jid, const QString &name, QXmppRosterIq::Item::SubscriptionType subscription, const QSet<QString> &groups = {});
    std::optional<RosterItem> fetchItem(const QString &jid);

    Database db;
    AccountDb *accountDb = nullptr;
    MessageDb *messageDb = nullptr;
    GroupChatUserDb *groupChatUserDb = nullptr;
    RosterDb *rosterDb = nullptr;
    AccountSettings *accountSettings = nullptr;
    std::unique_ptr<RosterStorage> storage;
};

RosterStorageTest::RosterStorageTest()
{
    AccountSettings::Data settingsData;
    settingsData.jid = accountJid;
    accountSettings = new AccountSettings(settingsData, this);

    accountDb = new AccountDb(this);
    messageDb = new MessageDb(this);
    groupChatUserDb = new GroupChatUserDb(this);
    rosterDb = new RosterDb(this);

    storage = std::make_unique<RosterStorage>(accountSettings);
}

void RosterStorageTest::initTestCase()
{
    Test::initTestCase();

    // Allow storing the account's password without a system keychain backend.
    QKeychainFuture::setUnencryptedFallback(true);

    // An account row is required so that the roster version can be persisted in it.
    AccountSettings::Data data;
    data.jid = accountJid;
    wait(accountDb->addAccount(data));
}

void RosterStorageTest::init()
{
    // Start each test with an empty roster and version.
    wait(this, storage->clear());
}

QXmppRosterIq::Item
RosterStorageTest::wireItem(const QString &jid, const QString &name, QXmppRosterIq::Item::SubscriptionType subscription, const QSet<QString> &groups)
{
    QXmppRosterIq::Item item;
    item.setBareJid(jid);
    item.setName(name);
    item.setSubscriptionType(subscription);
    item.setGroups(groups);
    return item;
}

std::optional<RosterItem> RosterStorageTest::fetchItem(const QString &jid)
{
    const auto items = wait(RosterDb::instance()->fetchItems());

    for (const auto &item : items) {
        if (item.accountJid == accountJid && item.jid == jid) {
            return item;
        }
    }

    return std::nullopt;
}

void RosterStorageTest::testUpsertAddsItem()
{
    wait(this, storage->upsertItem(QStringLiteral("v1"), wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both, {QStringLiteral("Friends")})));

    const auto item = fetchItem(alice);
    QVERIFY(item.has_value());
    QCOMPARE(item->name, QStringLiteral("Alice"));
    QCOMPARE(item->subscription, QXmppRosterIq::Item::Both);
    QCOMPARE(item->groups, QList<QString>{QStringLiteral("Friends")});
    // New items get the account's default encryption.
    QCOMPARE(item->encryption, accountSettings->encryption());
}

void RosterStorageTest::testUpsertPreservesConversationData()
{
    wait(this, storage->upsertItem(QStringLiteral("v1"), wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both)));

    // Attach conversation data that is not part of the roster wire format.
    wait(RosterDb::instance()->updateItem(accountJid, alice, [](RosterItem &item) {
        item.encryption = Encryption::NoEncryption;
        item.pinningPosition = 5;
        item.lastReadContactMessageId = QStringLiteral("msg-42");
        item.notificationRule = RosterItem::NotificationRule::Never;
    }));

    // A roster push that changes the wire data.
    wait(this,
         storage->upsertItem(QStringLiteral("v2"), wireItem(alice, QStringLiteral("Alice (renamed)"), QXmppRosterIq::Item::To, {QStringLiteral("Work")})));

    const auto item = fetchItem(alice);
    QVERIFY(item.has_value());

    // Wire data is overridden.
    QCOMPARE(item->name, QStringLiteral("Alice (renamed)"));
    QCOMPARE(item->subscription, QXmppRosterIq::Item::To);
    QCOMPARE(item->groups, QList<QString>{QStringLiteral("Work")});

    // Conversation data is preserved.
    QCOMPARE(item->encryption, Encryption::NoEncryption);
    QCOMPARE(item->pinningPosition, 5);
    QCOMPARE(item->lastReadContactMessageId, QStringLiteral("msg-42"));
    QCOMPARE(item->notificationRule, RosterItem::NotificationRule::Never);
}

void RosterStorageTest::testReplaceAllPreservesConversationData()
{
    wait(this, storage->upsertItem(QStringLiteral("v1"), wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both)));
    wait(this, storage->upsertItem(QStringLiteral("v2"), wireItem(bob, QStringLiteral("Bob"), QXmppRosterIq::Item::Both)));

    wait(RosterDb::instance()->updateItem(accountJid, alice, [](RosterItem &item) {
        item.pinningPosition = 9;
    }));

    // A full roster snapshot: Alice is updated, Bob is gone, Carol is new.
    const auto carol = QStringLiteral("carol@example.org");
    wait(this,
         storage->replaceAll(QStringLiteral("v3"),
                             {
                                 wireItem(alice, QStringLiteral("Alice (renamed)"), QXmppRosterIq::Item::To),
                                 wireItem(carol, QStringLiteral("Carol"), QXmppRosterIq::Item::Both),
                             }));

    const auto aliceItem = fetchItem(alice);
    QVERIFY(aliceItem.has_value());
    QCOMPARE(aliceItem->name, QStringLiteral("Alice (renamed)"));
    QCOMPARE(aliceItem->subscription, QXmppRosterIq::Item::To);
    // Conversation data of an existing item survives a full replace.
    QCOMPARE(aliceItem->pinningPosition, 9);

    QVERIFY(!fetchItem(bob).has_value());
    QVERIFY(fetchItem(carol).has_value());
}

void RosterStorageTest::testRemoveItem()
{
    wait(this, storage->upsertItem(QStringLiteral("v1"), wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both)));
    QVERIFY(fetchItem(alice).has_value());

    wait(this, storage->removeItem(QStringLiteral("v2"), alice));
    QVERIFY(!fetchItem(alice).has_value());
}

void RosterStorageTest::testClear()
{
    wait(this, storage->upsertItem(QStringLiteral("v1"), wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both)));
    wait(this, storage->upsertItem(QStringLiteral("v2"), wireItem(bob, QStringLiteral("Bob"), QXmppRosterIq::Item::Both)));

    wait(this, storage->clear());

    QVERIFY(!fetchItem(alice).has_value());
    QVERIFY(!fetchItem(bob).has_value());
    QVERIFY(wait(this, storage->load()).version.isEmpty());
}

void RosterStorageTest::testLoadRoundTrip()
{
    wait(this,
         storage->replaceAll(QStringLiteral("ver-123"),
                             {
                                 wireItem(alice, QStringLiteral("Alice"), QXmppRosterIq::Item::Both, {QStringLiteral("Friends")}),
                                 wireItem(bob, QStringLiteral("Bob"), QXmppRosterIq::Item::To),
                             }));

    const auto cache = wait(this, storage->load());
    QCOMPARE(cache.version, QStringLiteral("ver-123"));
    QCOMPARE(cache.items.size(), std::size_t(2));

    QMap<QString, QXmppRosterIq::Item> itemsByJid;
    for (const auto &item : cache.items) {
        itemsByJid.insert(item.bareJid(), item);
    }

    QVERIFY(itemsByJid.contains(alice));
    QCOMPARE(itemsByJid[alice].name(), QStringLiteral("Alice"));
    QCOMPARE(itemsByJid[alice].subscriptionType(), QXmppRosterIq::Item::Both);
    QCOMPARE(itemsByJid[alice].groups(), QSet<QString>{QStringLiteral("Friends")});

    QVERIFY(itemsByJid.contains(bob));
    QCOMPARE(itemsByJid[bob].subscriptionType(), QXmppRosterIq::Item::To);
}

void RosterStorageTest::testWireAttributesRoundTrip()
{
    // A contact with a pending ("ask") and pre-approved subscription.
    QXmppRosterIq::Item contact;
    contact.setBareJid(alice);
    contact.setName(QStringLiteral("Alice"));
    contact.setSubscriptionType(QXmppRosterIq::Item::From);
    contact.setSubscriptionStatus(QStringLiteral("subscribe"));
    contact.setIsApproved(true);
    contact.setGroups({QStringLiteral("Friends")});

    // A MIX channel.
    const auto channelJid = QStringLiteral("channel@mix.example.org");
    QXmppRosterIq::Item channel;
    channel.setBareJid(channelJid);
    channel.setIsMixChannel(true);
    channel.setMixParticipantId(QStringLiteral("part-1"));

    wait(this, storage->replaceAll(QStringLiteral("v1"), {contact, channel}));

    QMap<QString, QXmppRosterIq::Item> itemsByJid;
    for (const auto &item : wait(this, storage->load()).items) {
        itemsByJid.insert(item.bareJid(), item);
    }

    QVERIFY(itemsByJid.contains(alice));
    QCOMPARE(itemsByJid[alice].subscriptionType(), QXmppRosterIq::Item::From);
    QCOMPARE(itemsByJid[alice].subscriptionStatus(), QStringLiteral("subscribe"));
    QVERIFY(itemsByJid[alice].isApproved());

    QVERIFY(itemsByJid.contains(channelJid));
    QVERIFY(itemsByJid[channelJid].isMixChannel());
    QCOMPARE(itemsByJid[channelJid].mixParticipantId(), QStringLiteral("part-1"));
}

QTEST_GUILESS_MAIN(RosterStorageTest)

#include "RosterStorageTest.moc"
