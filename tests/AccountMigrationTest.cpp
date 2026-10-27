// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// std
#include <optional>
// Qt
#include <QBuffer>
#include <QDomDocument>
#include <QTemporaryDir>
#include <QTest>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
#include "AccountMigrationController.h"
#include "AccountMigrationData.h"
#include "Database.h"
#include "Encryption.h"
#include "GroupChatUserDb.h"
#include "Keychain.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterItem.h"
#include "Test.h"
#include "TestUtils.h"

static const auto accountJid = QStringLiteral("user@example.org");
static const auto alice = QStringLiteral("alice@example.org");
static const auto bob = QStringLiteral("bob@example.org");

class AccountMigrationTest : public Test
{
    Q_OBJECT

public:
    AccountMigrationTest();

private:
    Q_SLOT void initTestCase() override;

    Q_SLOT void testClientSettingsSerialization();
    Q_SLOT void testDiskRoundTrip();
    Q_SLOT void testImportClientSettingsWithMissingContact();

    static ClientSettings clientSettings();
    static void compareClientSettings(const ClientSettings &actual, const ClientSettings &expected);
    std::optional<RosterItem> fetchItem(const QString &jid);
    void addRosterItem(const QString &jid);

    Database db;
    AccountDb *accountDb = nullptr;
    MessageDb *messageDb = nullptr;
    GroupChatUserDb *groupChatUserDb = nullptr;
    RosterDb *rosterDb = nullptr;
    AccountMigrationController *controller = nullptr;
};

AccountMigrationTest::AccountMigrationTest()
{
    accountDb = new AccountDb(this);
    messageDb = new MessageDb(this);
    groupChatUserDb = new GroupChatUserDb(this);
    rosterDb = new RosterDb(this);

    // Registers ClientSettings as an extension of QXmppExportData.
    controller = new AccountMigrationController(this);
}

void AccountMigrationTest::initTestCase()
{
    Test::initTestCase();

    // Allow storing the account's password without a system keychain backend.
    QKeychainFuture::setUnencryptedFallback(true);

    AccountSettings::Data data;
    data.initialized = true;
    data.jid = accountJid;
    wait(accountDb->addAccount(data));
}

ClientSettings AccountMigrationTest::clientSettings()
{
    ClientSettings settings;
    settings.oldConfiguration.jid = QStringLiteral("old@example.org");
    settings.oldConfiguration.host = QStringLiteral("xmpp.example.org");
    settings.oldConfiguration.port = 5223;
    settings.oldConfiguration.encryption = Encryption::NoEncryption;
    settings.oldConfiguration.name = QStringLiteral("Old Account");

    ClientRosterItemSettings aliceSettings;
    aliceSettings.jid = alice;
    aliceSettings.encryption = Encryption::NoEncryption;
    aliceSettings.notificationRule = RosterItem::NotificationRule::Never;
    aliceSettings.pinningPosition = 3;

    ClientRosterItemSettings bobSettings;
    bobSettings.jid = bob;
    bobSettings.chatStateSendingEnabled = false;
    bobSettings.readMarkerSendingEnabled = false;
    bobSettings.pinningPosition = 7;

    settings.roster = {aliceSettings, bobSettings};
    return settings;
}

void AccountMigrationTest::compareClientSettings(const ClientSettings &actual, const ClientSettings &expected)
{
    QCOMPARE(actual.oldConfiguration.jid, expected.oldConfiguration.jid);
    QCOMPARE(actual.oldConfiguration.host, expected.oldConfiguration.host);
    QCOMPARE(actual.oldConfiguration.port, expected.oldConfiguration.port);
    QCOMPARE(actual.oldConfiguration.encryption, expected.oldConfiguration.encryption);
    QCOMPARE(actual.oldConfiguration.name, expected.oldConfiguration.name);

    QCOMPARE(actual.roster.size(), expected.roster.size());

    for (qsizetype i = 0; i < expected.roster.size(); ++i) {
        const auto &actualItem = actual.roster.at(i);
        const auto &expectedItem = expected.roster.at(i);

        QCOMPARE(actualItem.jid, expectedItem.jid);
        QCOMPARE(actualItem.encryption, expectedItem.encryption);
        QCOMPARE(actualItem.notificationRule, expectedItem.notificationRule);
        QCOMPARE(actualItem.chatStateSendingEnabled, expectedItem.chatStateSendingEnabled);
        QCOMPARE(actualItem.readMarkerSendingEnabled, expectedItem.readMarkerSendingEnabled);
        QCOMPARE(actualItem.pinningPosition, expectedItem.pinningPosition);
        QCOMPARE(actualItem.automaticMediaDownloadsRule, expectedItem.automaticMediaDownloadsRule);
    }
}

std::optional<RosterItem> AccountMigrationTest::fetchItem(const QString &jid)
{
    const auto items = wait(RosterDb::instance()->fetchItems());

    const auto itr = std::ranges::find_if(items, [&](const RosterItem &item) {
        return item.accountJid == accountJid && item.jid == jid;
    });

    if (itr == items.cend()) {
        return std::nullopt;
    }

    return *itr;
}

void AccountMigrationTest::addRosterItem(const QString &jid)
{
    RosterItem item;
    item.accountJid = accountJid;
    item.jid = jid;
    wait(this, rosterDb->updateOrAddItem(accountJid, {}, item));
}

void AccountMigrationTest::testClientSettingsSerialization()
{
    const auto expected = clientSettings();

    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    QXmlStreamWriter writer(&buffer);
    expected.toXml(writer);

    QDomDocument document;
    QVERIFY(document.setContent(buffer.data(), QDomDocument::ParseOption::UseNamespaceProcessing));

    const auto result = ClientSettings::fromDom(document.documentElement());
    QVERIFY(std::holds_alternative<ClientSettings>(result));

    compareClientSettings(std::get<ClientSettings>(result), expected);
}

void AccountMigrationTest::testDiskRoundTrip()
{
    const auto expected = clientSettings();

    QXmppExportData exportData;
    exportData.setAccountJid(QStringLiteral("old@example.org"));
    exportData.setExtension(ClientSettings(expected));

    QTemporaryDir dir;
    QVERIFY(dir.isValid());
    const auto filePath = dir.filePath(QStringLiteral("account.xml"));

    QVERIFY(AccountMigrationController::saveAccountDataToDisk(filePath, exportData));

    QXmppExportData restoredData;
    QVERIFY(AccountMigrationController::restoreAccountDataFromDisk(filePath, restoredData));

    QCOMPARE(restoredData.accountJid(), exportData.accountJid());

    const auto restoredSettings = restoredData.extension<ClientSettings>();
    QVERIFY(restoredSettings);
    compareClientSettings(*restoredSettings, expected);
}

void AccountMigrationTest::testImportClientSettingsWithMissingContact()
{
    // Only Alice is in the new account's roster, Bob is added later.
    addRosterItem(alice);

    const auto accounts = wait(accountDb->accounts());
    QCOMPARE(accounts.size(), 1);
    AccountSettings accountSettings(accounts.constFirst());

    const auto future = AccountMigrationController::importClientSettings(&accountSettings, clientSettings());
    wait(future);
    QVERIFY(future.isFinished());

    // The settings are applied to the account in use and stored.
    QCOMPARE(accountSettings.encryption(), Encryption::NoEncryption);
    QCOMPARE(accountSettings.name(), QStringLiteral("Old Account"));

    const auto storedAccounts = wait(accountDb->accounts());
    QCOMPARE(storedAccounts.size(), 1);
    QCOMPARE(storedAccounts.constFirst().encryption, Encryption::NoEncryption);
    QCOMPARE(storedAccounts.constFirst().name, QStringLiteral("Old Account"));

    const auto aliceItem = fetchItem(alice);
    QVERIFY(aliceItem);
    QCOMPARE(aliceItem->encryption, Encryption::NoEncryption);
    QCOMPARE(aliceItem->notificationRule, RosterItem::NotificationRule::Never);
    QCOMPARE(aliceItem->pinningPosition, 3);

    addRosterItem(bob);

    QTRY_COMPARE(fetchItem(bob)->pinningPosition, 7);

    const auto bobItem = fetchItem(bob);
    QCOMPARE(bobItem->chatStateSendingEnabled, false);
    QCOMPARE(bobItem->readMarkerSendingEnabled, false);
}

QTEST_GUILESS_MAIN(AccountMigrationTest)

#include "AccountMigrationTest.moc"
