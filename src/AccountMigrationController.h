// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QVariant>
#include <QtQml/qqmlregistration.h>
// QXmpp
#include <QXmppAccountMigrationManager.h>

struct ClientSettings;

class Account;
class AccountSettings;

class AccountMigrationController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Cannot create object; only enums defined!")

    friend class AccountMigrationTest;

public:
    explicit AccountMigrationController(QObject *parent = nullptr);
    ~AccountMigrationController() override;

    QFuture<bool> startMigration(Account *oldAccount);
    QFuture<void> finalizeMigration(Account *newAccount);

private:
    static void informUser(const QString &notification);

    QString diskAccountFilePath() const;
    static bool saveAccountDataToDisk(const QString &filePath, const QXmppExportData &data);
    static bool restoreAccountDataFromDisk(const QString &filePath, QXmppExportData &data);

    using ExportResult = std::variant<ClientSettings, QXmppError>;
    using ImportResult = std::variant<QXmpp::Success, QXmppError>;

    ClientSettings exportClientSettings();
    static QFuture<void> importClientSettings(AccountSettings *accountSettings, const ClientSettings &oldClientSettings);

    QFuture<QXmppAccountMigrationManager::Result<>> notifyContacts(Account *newAccount, const QList<QString> &contactJids);

    Account *m_oldAccount = nullptr;
    QXmppExportData m_exportData;
};
