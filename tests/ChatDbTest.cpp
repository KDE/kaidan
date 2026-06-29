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
#include "RosterItem.h"
#include "Test.h"
#include "TestUtils.h"

// Tests that ChatDb correctly splits a RosterItem across the two backing tables: the pure XMPP
// roster data ("roster") and the chat-list / conversation data ("chats"), while still exposing the
// fused RosterItem facade.
class ChatDbTest : public Test
{
    Q_OBJECT

private:
    Q_SLOT void initTestCase() override;
    Q_SLOT void cleanup();

    Q_SLOT void schemaSplit();
    Q_SLOT void addAndFetch();
    Q_SLOT void fieldsLandInCorrectTable();
    Q_SLOT void updateRoutesToCorrectTable();
    Q_SLOT void replaceItems();
    Q_SLOT void removeClearsBothTables();
    Q_SLOT void chatWithoutRosterEntryIsListed();

    RosterItem contact(const QString &jid) const;
    std::optional<RosterItem> fetch(const QString &jid) const;
    int rawCount(const char *table, const QString &jid) const;
    QVariant rawValue(const char *table, const QString &column, const QString &jid) const;

    std::unique_ptr<MainController> m_mainController;
    const QString m_accountJid = QStringLiteral("alice@example.org");
};

void ChatDbTest::initTestCase()
{
    Test::initTestCase();
    // Sets up the Database, MessageDb, ChatDb, GroupChatUserDb and RosterModel singletons.
    m_mainController = std::make_unique<MainController>();
}

void ChatDbTest::cleanup()
{
    // Keep the tests independent of each other.
    wait(ChatDb::instance()->removeItems(m_accountJid));
}

RosterItem ChatDbTest::contact(const QString &jid) const
{
    RosterItem item;
    item.accountJid = m_accountJid;
    item.jid = jid;
    return item;
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

// A full round-trip through the facade preserves fields from both tables.
void ChatDbTest::addAndFetch()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.name = QStringLiteral("Bob");
    item.subscription = QXmppRosterIq::Item::Both;
    item.groups = {QStringLiteral("Friends"), QStringLiteral("Work")};
    item.encryption = Encryption::Omemo2;
    item.notificationRule = RosterItem::NotificationRule::Always;
    item.pinningPosition = 3;

    wait(ChatDb::instance()->addItem(item));

    const auto fetched = fetch(item.jid);
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->name, QStringLiteral("Bob"));
    QCOMPARE(fetched->subscription, QXmppRosterIq::Item::Both);
    auto groups = fetched->groups;
    groups.sort();
    QCOMPARE(groups, QList<QString>({QStringLiteral("Friends"), QStringLiteral("Work")}));
    QCOMPARE(fetched->encryption, Encryption::Omemo2);
    QCOMPARE(fetched->notificationRule, RosterItem::NotificationRule::Always);
    QCOMPARE(fetched->pinningPosition, 3);
}

// Each field is physically stored in the table it belongs to.
void ChatDbTest::fieldsLandInCorrectTable()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.name = QStringLiteral("Bob");
    item.subscription = QXmppRosterIq::Item::To;
    item.encryption = Encryption::Omemo2;
    item.notificationRule = RosterItem::NotificationRule::Never;

    wait(ChatDb::instance()->addItem(item));

    QCOMPARE(rawCount(DB_TABLE_ROSTER, item.jid), 1);
    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 1);

    // pure roster data
    QCOMPARE(rawValue(DB_TABLE_ROSTER, QStringLiteral("name"), item.jid).toString(), QStringLiteral("Bob"));
    QCOMPARE(rawValue(DB_TABLE_ROSTER, QStringLiteral("subscription"), item.jid).toInt(), static_cast<int>(QXmppRosterIq::Item::To));
    // chat-list data
    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("encryption"), item.jid).toInt(), static_cast<int>(Encryption::Omemo2));
    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("notificationRule"), item.jid).toInt(), static_cast<int>(RosterItem::NotificationRule::Never));
}

// Updating fields routes the changes to the correct table.
void ChatDbTest::updateRoutesToCorrectTable()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.name = QStringLiteral("Bob");
    item.subscription = QXmppRosterIq::Item::None;
    item.encryption = Encryption::NoEncryption;
    wait(ChatDb::instance()->addItem(item));

    wait(ChatDb::instance()->updateItem(m_accountJid, item.jid, [](RosterItem &i) {
        i.name = QStringLiteral("Bobby"); // roster table
        i.subscription = QXmppRosterIq::Item::Both; // roster table
        i.encryption = Encryption::Omemo2; // chats table
        i.pinningPosition = 7; // chats table
    }));

    const auto fetched = fetch(item.jid);
    QVERIFY(fetched.has_value());
    QCOMPARE(fetched->name, QStringLiteral("Bobby"));
    QCOMPARE(fetched->subscription, QXmppRosterIq::Item::Both);
    QCOMPARE(fetched->encryption, Encryption::Omemo2);
    QCOMPARE(fetched->pinningPosition, 7);

    QCOMPARE(rawValue(DB_TABLE_ROSTER, QStringLiteral("name"), item.jid).toString(), QStringLiteral("Bobby"));
    QCOMPARE(rawValue(DB_TABLE_CHATS, QStringLiteral("pinningPosition"), item.jid).toInt(), 7);
}

// replaceItems() reconciles the roster, keeping/adding/removing entries (and their chat rows).
void ChatDbTest::replaceItems()
{
    auto bob = contact(QStringLiteral("bob@example.com"));
    bob.name = QStringLiteral("Bob");
    auto carol = contact(QStringLiteral("carol@example.com"));
    carol.name = QStringLiteral("Carol");
    wait(ChatDb::instance()->addItem(bob));
    wait(ChatDb::instance()->addItem(carol));

    // Keep bob (renamed), drop carol, add dave.
    auto bobRenamed = bob;
    bobRenamed.name = QStringLiteral("Bobby");
    auto dave = contact(QStringLiteral("dave@example.net"));
    dave.name = QStringLiteral("Dave");
    wait(ChatDb::instance()->replaceItems(m_accountJid, {bobRenamed, dave}));

    QVERIFY(fetch(bob.jid).has_value());
    QCOMPARE(fetch(bob.jid)->name, QStringLiteral("Bobby"));
    QVERIFY(fetch(dave.jid).has_value());
    QVERIFY(!fetch(carol.jid).has_value());

    // The removed contact is gone from both tables.
    QCOMPARE(rawCount(DB_TABLE_ROSTER, carol.jid), 0);
    QCOMPARE(rawCount(DB_TABLE_CHATS, carol.jid), 0);
}

void ChatDbTest::removeClearsBothTables()
{
    auto item = contact(QStringLiteral("bob@example.com"));
    item.name = QStringLiteral("Bob");
    wait(ChatDb::instance()->addItem(item));
    QCOMPARE(rawCount(DB_TABLE_ROSTER, item.jid), 1);
    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 1);

    wait(ChatDb::instance()->removeItem(m_accountJid, item.jid));

    QVERIFY(!fetch(item.jid).has_value());
    QCOMPARE(rawCount(DB_TABLE_ROSTER, item.jid), 0);
    QCOMPARE(rawCount(DB_TABLE_CHATS, item.jid), 0);
}

// Future MUC group chats are chats without a roster entry: a chats row with no matching roster row
// must still be listed (the read joins chats LEFT JOIN roster).
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
