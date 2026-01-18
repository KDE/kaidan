// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QObject>
#include <QPointer>
// Kaidan
#include "Account.h"

class AccountMigrationController;
class ClientController;

class AccountController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<Account *> accounts READ accounts NOTIFY accountsChanged)
    Q_PROPERTY(bool migrating READ migrating NOTIFY migratingChanged)

public:
    static AccountController *instance();

    explicit AccountController(QObject *parent = nullptr);
    ~AccountController();

    Q_INVOKABLE Account *createUninitializedAccount();
    Q_INVOKABLE void discardUninitializedAccount(Account *account);

    Q_INVOKABLE Account *account(const QString &jid) const;
    QList<Account *> accounts() const;
    Q_SIGNAL void accountsChanged();

    Q_INVOKABLE QList<Account *> enabledAccounts() const;

    Q_SIGNAL void accountAvailable();
    Q_SIGNAL void noAccountAvailable();

    Q_INVOKABLE void setActiveAccount(Account *account);
    Q_INVOKABLE void resetActiveAccount();

    Q_INVOKABLE void startMigration(Account *account);
    bool migrating();
    Q_SIGNAL void migratingChanged();

    Q_INVOKABLE bool logOutAllAccountsToQuit();
    Q_SIGNAL void allAccountsLoggedOutToQuit();

    Q_SIGNAL void accountAdded(Account *account);
    Q_SIGNAL void accountRemoved(const QString &jid);

private:
    QFuture<void> loadAccounts();
    void handleAccountAdded(const AccountSettings::Data &accountSettings);
    void removeAccount(const QString &jid);

    void cancelMigration();

    QList<Account *> m_accounts;

    Account *m_activeAccount = nullptr;
    QPointer<Account> m_accountToBeDeleted;
    quint32 m_accountsLoggingOutCount = 0;

    QPointer<AccountMigrationController> m_migrationController;

    static AccountController *s_instance;
};
