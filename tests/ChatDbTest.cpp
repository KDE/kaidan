// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Qt
#include <QSqlQuery>
#include <QSqlRecord>
#include <QTest>
// Kaidan
#include "ChatDb.h"
#include "Database.h"
#include "Globals.h"
#include "MainController.h"
#include "RosterDb.h"
#include "RosterItem.h"
#include "Test.h"
#include "TestUtils.h"

// Tests that ChatDb owns the chat-list / conversation data ("chats") while the pure XMPP roster data
// ("roster") is owned by RosterDb, and that ChatDb still exposes the fused RosterItem facade by
// joining both tables.
class ChatDbTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void initTestCase() override;
    Q_SLOT void cleanup();

    Q_SLOT void schemaSplit();
    Q_SLOT void fusedRead();
    Q_SLOT void chatFieldsLandInChatsTable();
    Q_SLOT void updateWritesOnlyChats();
    Q_SLOT void replaceItemsReconcilesChats();
    Q_SLOT void removeClearsChatRow();
    Q_SLOT void chatWithoutRosterEntryIsListed();

    RosterItem contact(const QString &jid) const;
    void seedRoster(const QString &jid, const QString &name, QXmppRosterIq::Item::SubscriptionType subscription, const QSet<QString> &groups = {});
    std::optional<RosterItem> fetch(const QString &jid) const;
    int rawCount(const char *table, const QString &jid) const;
    QVariant rawValue(const char *table, const QString &column, const QString &jid) const;

    std::unique_ptr<MainController> m_mainController;
    const QString m_accountJid = QStringLiteral("alice@example.org");
};

void ChatDbTest::initTestCase()
{
    Test::initTestCase();
    // Sets up the Database, MessageDb, ChatDb, RosterDb, GroupChatUserDb and RosterModel singletons.
    m_mainController = std::make_unique<MainController>();
}

void ChatDbTest::cleanup()
{
    // Keep the tests independent of each other.
    wait(ChatDb::instance()->removeItems(m_accountJid));
    wait(this, RosterDb::instance()->clear(m_accountJid));
}

RosterItem ChatDbTest::contact(const QString &jid) const
{
    RosterItem item;
    item.accountJid = m_accountJid;
    item.jid = jid;
    return item;
}

void ChatDbTest::seedRoster(const QString &jid, const QString &name, QXmppRosterIq::Item::SubscriptionType subscription, const QSet<QString> &groups)
{
    QXmppRosterIq::Item item;
    item.setBareJid(jid);
    item.setName(name);
    item.setSubscriptionType(subscription);
    item.setGroups(groups);
    wait(this, RosterDb::instance()->upsertItem(m_accountJid, QStringLiteral("v1"), item));
}

std::optional<RosterItem> ChatDbTest::fetch(const QString &jid) const
{
    const auto items = wait(ChatDb::instance()->fetchItems());
    for (const auto &item : items) {
        if (item.accountJid == m_accountJid && item.jid == jid) {
            return item;
        }
    }
    return std::nullopt;
}

int ChatDbTest::rawCount(const char *table, const QString &jid) const
{
    auto db = Database::instance()->currentDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT COUNT(*) FROM %1 WHERE accountJid = ? AND jid = ?").arg(QString::fromLatin1(table)));
    query.addBindValue(m_accountJid);
    query.addBindValue(jid);
    [&] {
        QVERIFY(query.exec());
        QVERIFY(query.next());
    }();
    return query.value(0).toInt();
}

QVariant ChatDbTest::rawValue(const char *table, const QString &column, const QString &jid) const
{
    auto db = Database::instance()->currentDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("SELECT %1 FROM %2 WHERE accountJid = ? AND jid = ?").arg(column, QString::fromLatin1(table)));
    query.addBindValue(m_accountJid);
    query.addBindValue(jid);
    [&] {
        QVERIFY(query.exec());
        QVERIFY(query.next());
    }();
    return query.value(0);
}

// The roster table only holds pure XMPP roster fields; everything else lives in the chats table.
void ChatDbTest::schemaSplit()
{
    // Ensure the (asynchronous) table creation has finished before inspecting the schema.
    wait(ChatDb::instance()->fetchItems());

    auto db = Database::instance()->currentDatabase();

    const auto fieldNames = [&](const char *table) {
        QSqlQuery query(db);
        query.exec(QStringLiteral("PRAGMA table_info(%1)").arg(QString::fromLatin1(table)));
        QStringList names;
        while (query.next()) {
            names.append(query.value(1).toString());
        }
        names.sort();
        return names;
    };

    QStringList expectedRoster{
        QStringLiteral("accountJid"),
        QStringLiteral("jid"),
        QStringLiteral("name"),
        QStringLiteral("subscription"),
        QStringLiteral("groupChatParticipantId"),
        QStringLiteral("subscriptionStatus"),
        QStringLiteral("approved"),
        QStringLiteral("isMixChannel"),
    };
    expectedRoster.sort();
    QCOMPARE(fieldNames(DB_TABLE_ROSTER), expectedRoster);

    const auto chatsFields = fieldNames(DB_TABLE_CHATS);
    // chat-list data is in the chats table ...
    QVERIFY(chatsFields.contains(QStringLiteral("encryption")));
    QVERIFY(chatsFields.contains(QStringLiteral("pinningPosition")));
    QVERIFY(chatsFields.contains(QStringLiteral("notificationRule")));
    QVERIFY(chatsFields.contains(QStringLiteral("groupChatName")));
    // ... and the pure roster fields are not duplicated there.
    QVERIFY(!chatsFields.contains(QStringLiteral("name")));
    QVERIFY(!chatsFields.contains(QStringLiteral("subscription")));
    QVERIFY(!chatsFields.contains(QStringLiteral("groupChatParticipantId")));
}

// The fused RosterItem facade combines the roster data (owned by RosterDb) with the chat-list data
// (owned by ChatDb) via the LEFT JOIN.
void ChatDbTest::fusedRead()
{
    // chat-list data written by ChatDb
    auto item = contact(QStringLiteral("bob@example.com"));
    item.encryption = Encryption::Omemo2;
    item.notificationRule = RosterItem::NotificationRule::Always;
    item.pinningPosition = 3;
    wait(ChatDb::instance()->addItem(item));

    // roster data written by RosterDb (the SQL QXmppRosterStorage backend)
    seedRoster(item.jid, QStringLiteral("Bob"), QXmppRosterIq::Item::Both, {QStringLiteral("Friends"), QStringLiteral("Work")});

    const auto fetched = fetch(item.jid);
    QVERIFY(fetched.has_value());
    // roster fields come from RosterDb's roster row
    QCOMPARE(fetched->name, QStringLiteral("Bob"));
    QCOMPARE(fetched->subscription, QXmppRosterIq::Item::Both);
    auto groups = fetched->groups;
    groups.sort();
    QCOMPARE(groups, QList<QString>({QStringLiteral("Friends"), QStringLiteral("Work")}));
    // chat-list fields come from ChatDb's chats row
    QCOMPARE(fetched->encryption, Encryption::Omemo2);
    QCOMPARE(fetched->notificationRule, RosterItem::NotificationRule::Always);
    QCOMPARE(fetched->pinningPosition, 3);
}

// ChatDb writes only the chats row; roster fields passed in the RosterItem are ignored (RosterDb owns
// the roster table).
void ChatDbTest::chatFieldsLandInChatsTable()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.name = QStringLiteral("Bob"); // roster field -> ignored by ChatDb
    item.subscription = QXmppRosterIq::Item::To; // roster field -> ignored by ChatDb
    item.encryption = Encryption::Omemo2;
    item.notificationRule = RosterItem::NotificationRule::Never;
    wait(ChatDb::instance()->addItem(item));

    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 1);
    QCOMPARE(rawCount(DB_TABLE_ROSTER, item.jid), 0);

    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("encryption"), item.jid).toInt(), static_cast<int>(Encryption::Omemo2));
    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("notificationRule"), item.jid).toInt(), static_cast<int>(RosterItem::NotificationRule::Never));
}

// Updating chat-list fields routes the changes to the chats table; ChatDb never writes roster data.
void ChatDbTest::updateWritesOnlyChats()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.encryption = Encryption::NoEncryption;
    wait(ChatDb::instance()->addItem(item));

    wait(ChatDb::instance()->updateItem(m_accountJid, item.jid, [](RosterItem &i) {
        i.encryption = Encryption::Omemo2; // chats table
        i.pinningPosition = 7; // chats table
    }));

    const auto fetched = fetch(item.jid);
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->encryption, Encryption::Omemo2);
    QCOMPARE(fetched->pinningPosition, 7);

    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("pinningPosition"), item.jid).toInt(), 7);
    QCOMPARE(rawCount(DB_TABLE_ROSTER, item.jid), 0);
}

// replaceItems() reconciles the chats rows against the new roster: keeping, adding and removing.
void ChatDbTest::replaceItemsReconcilesChats()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    auto carol = contact(QStringLiteral("carol@example.com"));
    wait(ChatDb::instance()->addItem(bob));
    wait(ChatDb::instance()->addItem(carol));

    // Keep bob, drop carol, add dave.
    auto dave = contact(QStringLiteral("dave@example.net"));
    wait(ChatDb::instance()->replaceItems(m_accountJid, {bob, dave}));

    QVERIFY(fetch(bob.jid).has_value());
    QVERIFY(fetch(dave.jid).has_value());
    QVERIFY(!fetch(carol.jid).has_value());

    QCOMPARE(rawCount(DB_TABLE_CHATS, bob.jid), 1);
    QCOMPARE(rawCount(DB_TABLE_CHATS, dave.jid), 1);
    QCOMPARE(rawCount(DB_TABLE_CHATS, carol.jid), 0);
}

void ChatDbTest::removeClearsChatRow()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    wait(ChatDb::instance()->addItem(item));
    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 1);

    wait(ChatDb::instance()->removeItem(m_accountJid, item.jid));

    QVERIFY(!fetch(item.jid).has_value());
    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 0);
}

// A chats row with no matching roster row (e.g. a future MUC group chat bookmark) must still be
// listed (the read joins chats LEFT JOIN roster).
void ChatDbTest::chatWithoutRosterEntryIsListed()
{
    const auto jid = QStringLiteral("room@conference.example.org");

    auto db = Database::instance()->currentDatabase();
    QSqlQuery query(db);
    query.prepare(QStringLiteral("INSERT INTO " DB_TABLE_CHATS " (accountJid, jid, groupChatName, readMarkerPending, pinningPosition, "
                                 "chatStateSendingEnabled, readMarkerSendingEnabled, notificationRule, automaticMediaDownloadsRule) "
                                 "VALUES (?, ?, ?, 0, -1, 1, 1, 0, 0)"));
    query.addBindValue(m_accountJid);
    query.addBindValue(jid);
    query.addBindValue(QStringLiteral("Some Room"));
    QVERIFY(query.exec());

    const auto fetched = fetch(jid);
    QVERIFY(fetched.has_value());
    // No roster entry, so the pure roster fields come back empty (joined as NULL).
    QVERIFY(fetched->name.isEmpty());
    QVERIFY(fetched->groupChatParticipantId.isEmpty());
    // The chat-list data is still there.
    QCOMPARE(fetched->groupChatName, QStringLiteral("Some Room"));

    // Clean up the manually inserted row (cleanup() only removes proper roster entries).
    QSqlQuery deleteQuery(db);
    deleteQuery.prepare(QStringLiteral("DELETE FROM " DB_TABLE_CHATS " WHERE accountJid = ? AND jid = ?"));
    deleteQuery.addBindValue(m_accountJid);
    deleteQuery.addBindValue(jid);
    QVERIFY(deleteQuery.exec());
}

QTEST_GUILESS_MAIN(ChatDbTest)
#include "ChatDbTest.moc"
