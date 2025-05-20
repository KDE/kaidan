// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountController.h"

// Kaidan
#include "AccountDb.h"
#include "AccountMigrationController.h"
#include "Algorithms.h"
#include "MainController.h"

AccountController *AccountController::s_instance = nullptr;

AccountController *AccountController::instance()
{
    return s_instance;
}

AccountController::AccountController(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(this, &AccountController::accountAvailable, this, &AccountController::accountsChanged);
    connect(this, &AccountController::accountAdded, this, &AccountController::accountsChanged);
    connect(this, &AccountController::accountRemoved, this, &AccountController::accountsChanged);

    connect(AccountDb::instance(), &AccountDb::accountAdded, this, &AccountController::handleAccountAdded);
    connect(AccountDb::instance(), &AccountDb::accountRemoved, this, &AccountController::removeAccount);

    loadAccounts();
}

AccountController::~AccountController()
{
    s_instance = nullptr;
}

Account *AccountController::createAccount()
{
    auto *account = new Account(this);
    m_accounts.append(account);
    return account;
}

void AccountController::discardUninitializedAccount(Account *account)
{
    if (!account->settings()->initialized()) {
        account->deleteLater();
        m_accounts.removeOne(account);

        if (migrating()) {
            cancelMigration();
        }
    }
}

Account *AccountController::account(const QString &jid) const
{
    auto itr = std::ranges::find_if(m_accounts, [jid](const Account *account) {
        return account->settings()->jid() == jid;
    });

    Q_ASSERT(itr != m_accounts.end());

    return *itr;
}

QList<Account *> AccountController::accounts() const
{
    return m_accounts;
}

QList<Account *> AccountController::enabledAccounts() const
{
    return filter(QList<Account *>{m_accounts}, [](Account *account) {
        return account->settings()->enabled();
    });
}

void AccountController::setActiveAccount(Account *activeAccount)
{
    m_activeAccount = activeAccount;
}

void AccountController::resetActiveAccount()
{
    if (m_accountToBeDeleted) {
        m_accountToBeDeleted->deleteLater();
    }

    m_activeAccount = nullptr;
}

void AccountController::startMigration(Account *account)
{
    Q_ASSERT(account);
    Q_ASSERT(!m_migrationController);

    m_migrationController = new AccountMigrationController(this);
    Q_EMIT migratingChanged();

    m_migrationController->startMigration(account).then([this](bool started) {
        if (started) {
            Q_EMIT MainController::instance()->openStartPageRequested();
        } else {
            cancelMigration();
        }
    });
}

bool AccountController::migrating()
{
    return m_migrationController;
}

bool AccountController::logOutAllAccountsToQuit()
{
    for (auto *account : m_accounts) {
        if (auto *connection = account->connection(); connection->state() != Enums::ConnectionState::StateDisconnected) {
            m_accountsLoggingOutCount++;

            connect(connection, &Connection::stateChanged, this, [this, connection]() {
                if (connection->state() == Enums::ConnectionState::StateDisconnected) {
                    if (--m_accountsLoggingOutCount == 0) {
                        Q_EMIT allAccountsLoggedOutToQuit();
                    }
                }
            });
        }
    }

    bool loggingOut = m_accountsLoggingOutCount;

    for (auto *account : m_accounts) {
        if (auto *connection = account->connection(); connection->state() != Enums::ConnectionState::StateDisconnected) {
            connection->logOut(true);
        }
    }

    return loggingOut;
}

QFuture<void> AccountController::loadAccounts()
{
    return AccountDb::instance()->accounts().then(this, [this](const QList<AccountSettings::Data> &loadedAccountSettings) {
        for (const auto &accountSettings : loadedAccountSettings) {
            auto *account = new Account(accountSettings, this);
            account->restoreState();
            m_accounts.append(account);
        }

        if (loadedAccountSettings.isEmpty()) {
            Q_EMIT noAccountAvailable();
        } else {
            Q_EMIT accountAvailable();
        }
    });
}

void AccountController::handleAccountAdded(const AccountSettings::Data &accountSettings)
{
    auto itr = std::ranges::find_if(m_accounts, [jid = accountSettings.jid](const Account *account) {
        return account->settings()->jid() == jid;
    });

    if (itr != m_accounts.end()) {
        auto *account = *itr;

        if (m_migrationController) {
            connect(
                account->connection(),
                &Connection::connected,
                this,
                [this, account]() {
                    m_migrationController->finalizeMigration(account).then(this, [this]() {
                        cancelMigration();
                    });
                },
                Qt::SingleShotConnection);
        }

        Q_EMIT accountAdded(account);
    }
}

void AccountController::removeAccount(const QString &jid)
{
    auto itr = std::ranges::find_if(m_accounts, [jid](const Account *account) {
        return account->settings()->jid() == jid;
    });

    if (itr != m_accounts.end()) {
        auto *account = *itr;
        m_accounts.erase(itr);
        Q_EMIT accountRemoved(jid);

        if (m_activeAccount) {
            m_accountToBeDeleted = account;
        } else {
            account->deleteLater();
        }

        if (m_accounts.isEmpty()) {
            Q_EMIT noAccountAvailable();
        }
    }
}

void AccountController::cancelMigration()
{
    m_migrationController->deleteLater();
    Q_EMIT migratingChanged();
}

#include "moc_AccountController.cpp"
