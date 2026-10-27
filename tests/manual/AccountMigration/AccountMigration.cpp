// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

// Manual end-to-end test of the account migration against a local XMPP server.
//
// It logs into old@localhost, adds contacts with settings, starts the migration, logs into
// new@localhost like the start page does and checks the migrated roster and settings.
// Start the server with run-server.sh, which prints the command to run this test.
//
// Usage: AccountMigration <CA certificate of the server>

// Qt
#include <QGuiApplication>
#include <QSslConfiguration>
#include <QTemporaryDir>
#include <QTest>
// QXmpp
#include <QXmppRosterManager.h>
// Kaidan
#include "Account.h"
#include "AccountController.h"
#include "ClientController.h"
#include "Keychain.h"
#include "MainController.h"
#include "RosterDb.h"
#include "RosterModel.h"

static const auto password = QStringLiteral("secret");

static void log(const QString &msg)
{
    fprintf(stderr, "[AccountMigration] %s\n", qPrintable(msg));
}

static bool waitFor(const std::function<bool()> &pred, int ms, const QString &what)
{
    const bool ok = QTest::qWaitFor(pred, ms);
    log(QStringLiteral("%1: %2").arg(what, ok ? QStringLiteral("OK") : QStringLiteral("TIMEOUT")));
    return ok;
}

static Account *logIn(const QString &jid)
{
    auto *account = AccountController::instance()->createUninitializedAccount();
    account->settings()->setJid(jid);
    account->settings()->setPassword(password);
    account->connection()->logIn();
    if (!waitFor(
            [account] {
                return account->connection()->state() == Enums::ConnectionState::StateConnected;
            },
            15000,
            QStringLiteral("log in %1").arg(jid))) {
        return nullptr;
    }
    return account;
}

static std::optional<RosterItem> rosterItem(const QString &accountJid, const QString &jid)
{
    const auto items = RosterModel::instance()->items(accountJid);
    const auto itr = std::ranges::find_if(items, [&](const RosterItem &item) {
        return item.jid == jid;
    });
    return itr == items.cend() ? std::nullopt : std::optional(*itr);
}

static QStringList serverRoster(Account *account)
{
    return account->clientController()->rosterManager()->getRosterBareJids();
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <CA certificate of the server>\n", argv[0]);
        return 1;
    }

    // Use a separate database and settings instead of the user's ones.
    QTemporaryDir home;
    qputenv("XDG_DATA_HOME", home.filePath(QStringLiteral("data")).toUtf8());
    qputenv("XDG_CONFIG_HOME", home.filePath(QStringLiteral("config")).toUtf8());
    qputenv("XDG_CACHE_HOME", home.filePath(QStringLiteral("cache")).toUtf8());
    qputenv("QT_QPA_PLATFORM", "offscreen");

    QGuiApplication app(argc, argv);
    QGuiApplication::setApplicationName(QStringLiteral("kaidan-account-migration-test"));

    auto ssl = QSslConfiguration::defaultConfiguration();
    ssl.addCaCertificates(QString::fromLocal8Bit(argv[1]));
    QSslConfiguration::setDefaultConfiguration(ssl);

    QKeychainFuture::setUnencryptedFallback(true);

    MainController mainController(nullptr);
    QObject::connect(&mainController, &MainController::passiveNotificationRequested, [](const QString &text) {
        log(QStringLiteral("notification: %1").arg(text));
    });
    bool startPageRequested = false;
    QObject::connect(&mainController, &MainController::openStartPageRequested, [&] {
        log(QStringLiteral("start page requested (export finished)"));
        startPageRequested = true;
    });

    const auto oldJid = QStringLiteral("old@localhost");
    const auto newJid = QStringLiteral("new@localhost");
    const auto alice = QStringLiteral("alice@localhost");
    const auto bob = QStringLiteral("bob@localhost");

    auto *oldAccount = logIn(oldJid);
    if (!oldAccount) {
        return 1;
    }

    // Prepare the old account: two contacts on the server, local settings for them.
    auto *rosterManager = oldAccount->clientController()->rosterManager();
    rosterManager->addRosterItem(alice, QStringLiteral("Alice"), {QStringLiteral("Friends")});
    rosterManager->addRosterItem(bob, QStringLiteral("Bob"));
    waitFor(
        [&] {
            return rosterItem(oldJid, alice) && rosterItem(oldJid, bob);
        },
        10000,
        QStringLiteral("old account has alice and bob"));

    RosterDb::instance()->updateItem(oldJid, alice, [](RosterItem &item) {
        item.pinningPosition = 4;
    });
    RosterDb::instance()->updateItem(oldJid, bob, [](RosterItem &item) {
        item.chatStateSendingEnabled = false;
    });
    waitFor(
        [&] {
            return rosterItem(oldJid, alice)->pinningPosition == 4 && !rosterItem(oldJid, bob)->chatStateSendingEnabled;
        },
        5000,
        QStringLiteral("old account settings stored"));

    // Migration: export from the old account.
    AccountController::instance()->startMigration(oldAccount);
    if (!waitFor(
            [&] {
                return startPageRequested || !AccountController::instance()->migrating();
            },
            20000,
            QStringLiteral("export"))) {
        return 1;
    }
    if (!AccountController::instance()->migrating()) {
        log(QStringLiteral("migration was cancelled during export"));
        return 1;
    }

    // Log into the new account like the start page does, which finalizes the migration.
    auto *newAccount = logIn(newJid);
    if (!newAccount) {
        return 1;
    }

    const bool finished = waitFor(
        [] {
            return !AccountController::instance()->migrating();
        },
        30000,
        QStringLiteral("migration finished"));

    // Give roster pushes and settings updates some time.
    QTest::qWait(3000);

    log(QStringLiteral("new account server roster: %1").arg(serverRoster(newAccount).join(u", ")));
    const auto newAlice = rosterItem(newJid, alice);
    const auto newBob = rosterItem(newJid, bob);
    log(QStringLiteral("new alice: %1, pinningPosition=%2").arg(bool(newAlice)).arg(newAlice ? newAlice->pinningPosition : -2));
    log(QStringLiteral("new bob: %1, chatStateSendingEnabled=%2").arg(bool(newBob)).arg(newBob ? newBob->chatStateSendingEnabled : -1));

    const auto aliceEntry = newAccount->clientController()->rosterManager()->getRosterEntry(alice);
    log(QStringLiteral("new alice roster entry: name=%1, groups=%2")
            .arg(aliceEntry.name(), QStringList(aliceEntry.groups().cbegin(), aliceEntry.groups().cend()).join(u", ")));

    const bool ok = finished && newAlice && newBob && newAlice->pinningPosition == 4 && !newBob->chatStateSendingEnabled
        && aliceEntry.name() == QStringLiteral("Alice") && aliceEntry.groups().contains(QStringLiteral("Friends"));
    log(ok ? QStringLiteral("RESULT: SUCCESS") : QStringLiteral("RESULT: FAILURE"));
    return ok ? 0 : 1;
}
