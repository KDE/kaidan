// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QXmppAccountMigrationManager.h>

#include <QObject>
#include <QVariant>

#include <memory>
#include <optional>

class ClientWorker;
class RosterItem;
class Settings;

class AccountMigrationManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool busy READ isBusy NOTIFY busyChanged FINAL)
	Q_PROPERTY(AccountMigrationManager::MigrationState migrationState READ migrationState NOTIFY migrationStateChanged FINAL)

public:
	enum class MigrationState {
		Idle = 0,
		Started,
		Exporting,
		Registering,
		Importing,
		Finished,
	};
	Q_ENUM(MigrationState)

	explicit AccountMigrationManager(ClientWorker *clientWorker, QObject *parent = nullptr);
	~AccountMigrationManager() override;

	AccountMigrationManager::MigrationState migrationState() const;

	Q_SLOT void startMigration();
	Q_SLOT void continueMigration(const QVariant &userData = {});
	Q_SLOT void cancelMigration();
	Q_SIGNAL void migrationStateChanged(AccountMigrationManager::MigrationState state);

	bool isBusy() const;
	Q_SIGNAL void busyChanged(bool busy);

	Q_SIGNAL void errorOccurred(const QString &error);

private:
	QString diskAccountFilePath() const;
	bool saveAccountDataToDisk(const QXmppExportData &data);
	bool restoreAccountDataFromDisk(QXmppExportData &data);

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

	struct MigrationData : AbstractData<AccountMigrationManager::MigrationState> {
		QXmppExportData account;
	};

	ClientWorker *const m_worker;
	std::unique_ptr<QXmppAccountMigrationManager> m_manager;
	std::optional<MigrationData> m_migrationData;
};

Q_DECLARE_METATYPE(AccountMigrationManager::MigrationState)
