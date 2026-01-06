// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QVariant>
// QXmpp
#include <QXmppAccountMigrationManager.h>

struct ClientSettings;

class Account;

class AccountMigrationController : public QObject
{
    Q_OBJECT

public:
    explicit AccountMigrationController(QObject *parent = nullptr);
    ~AccountMigrationController() override;

    QFuture<bool> startMigration(Account *oldAccount);
    QFuture<void> finalizeMigration(Account *newAccount);

private:
    void informUser(const QString &notification);

    QString diskAccountFilePath() const;
    bool saveAccountDataToDisk(const QString &filePath, const QXmppExportData &data);
    bool restoreAccountDataFromDisk(const QString &filePath, QXmppExportData &data);

    using ExportResult = std::variant<ClientSettings, QXmppError>;
    using ImportResult = std::variant<QXmpp::Success, QXmppError>;

    ClientSettings exportClientSettings();
    QFuture<void> importClientSettings(Account *newAccount, const ClientSettings &oldClientSettings);

    QFuture<QXmppAccountMigrationManager::Result<>> notifyContacts(Account *newAccount, const QList<QString> &contactJids);

    Account *m_oldAccount;
    QXmppExportData m_exportData;
};
