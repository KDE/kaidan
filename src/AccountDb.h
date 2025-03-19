// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "DatabaseComponent.h"

class Account;

class AccountDb : public DatabaseComponent
{
    Q_OBJECT

    friend class Database;

public:
    AccountDb(Database *db, QObject *parent = nullptr);
    ~AccountDb();

    static AccountDb *instance();

    QFuture<void> addAccount(const QString &jid);
    QFuture<Account> account(const QString &jid);
    QFuture<Account> lastAccount();
    QFuture<void> updateAccount(const QString &jid, const std::function<void(Account &)> &updateAccount);

    /**
     * Fetches the stanza ID of the latest locally stored (existing or removed) message.
     */
    QFuture<QString> fetchLatestMessageStanzaId(const QString &jid);

    QFuture<void> removeAccount(const QString &jid);

private:
    static void parseAccountsFromQuery(QSqlQuery &query, QList<Account> &accounts);
    static QSqlRecord createUpdateRecord(const Account &oldAccount, const Account &newAccount);
    void updateAccountByRecord(const QString &jid, const QSqlRecord &record);

    static AccountDb *s_instance;
};
