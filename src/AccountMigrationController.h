// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QVariant>
// QXmpp
#include <QXmppAccountMigrationManager.h>

class ClientWorker;
class QXmppClient;
class QXmppConfiguration;
class QXmppMovedManager;

struct ClientSettings;

class AccountMigrationController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged FINAL)
    Q_PROPERTY(AccountMigrationController::MigrationState migrationState READ migrationState NOTIFY migrationStateChanged FINAL)

public:
    enum class MigrationState {
        Idle = 0,
        Started,
        Exporting,
        ChoosingNewAccount,
        Importing,
        Finished,
    };
    Q_ENUM(MigrationState)

    explicit AccountMigrationController(QObject *parent = nullptr);
    ~AccountMigrationController() override;

    AccountMigrationController::MigrationState migrationState() const;
    Q_SIGNAL void migrationStateChanged();

    Q_INVOKABLE void startMigration();
    Q_INVOKABLE void cancelMigration();

    bool busy() const;
    Q_SIGNAL void busyChanged();

private:
    void continueMigration(const QVariant &userData = {});
    void handleError(const QString &error);

    QFuture<QString> diskAccountFilePath() const;
    bool saveAccountDataToDisk(const QString &filePath, const QXmppExportData &data);
    bool restoreAccountDataFromDisk(const QString &filePath, QXmppExportData &data);

    using ImportResult = std::variant<QXmpp::Success, QXmppError>;
    using ExportResult = std::variant<ClientSettings, QXmppError>;

    QFuture<void> importClientSettings(const ClientSettings &settings);
    ClientSettings exportClientSettings();

    QXmppTask<QXmppAccountMigrationManager::Result<>> publishMovedStatement(const QXmppConfiguration &configuration, const QString &newAccountJid);
    QXmppTask<QXmppAccountMigrationManager::Result<>> notifyContacts(const QList<QString> &contactJids, const QString &oldAccountJid);

    template<typename Enum>
    struct AbstractData {
        using State = Enum;

        State state = State::Idle;
        QString error;

        void advanceState()
        {
            using Integral = std::underlying_type_t<Enum>;
            const Integral next = static_cast<Integral>(state) + 1;
            Q_ASSERT(next <= static_cast<Integral>(Enum::Finished));
            state = static_cast<Enum>(next);
        }
    };

    struct MigrationData : AbstractData<AccountMigrationController::MigrationState> {
        QXmppExportData account;
    };

    ClientWorker *const m_clientWorker;
    QXmppClient *const m_client;
    QXmppAccountMigrationManager *const m_migrationManager;
    QXmppMovedManager *const m_movedManager;
    std::optional<MigrationData> m_migrationData;
};

Q_DECLARE_METATYPE(AccountMigrationController::MigrationState)
