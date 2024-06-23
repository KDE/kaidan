// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationManager.h"
#include "Algorithms.h"
#include "Encryption.h"
#include "RegistrationManager.h"
#include "RosterItem.h"
#include "Settings.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <QXmppAccountMigrationManager.h>

struct ClientRosterItemSettings {
	ClientRosterItemSettings() = default;
	explicit ClientRosterItemSettings(const RosterItem &item)
	{
		bareJid = item.jid;
		encryption = item.encryption;
		notificationsMuted = item.notificationsMuted;
		chatStateSendingEnabled = item.chatStateSendingEnabled;
		readMarkerSendingEnabled = item.readMarkerSendingEnabled;
		pinningPosition = item.pinningPosition;
		automaticMediaDownloadsRule = item.automaticMediaDownloadsRule;
	}

	QString bareJid;
	Encryption::Enum encryption;
	bool notificationsMuted;
	bool chatStateSendingEnabled;
	bool readMarkerSendingEnabled;
	int pinningPosition;
	RosterItem::AutomaticMediaDownloadsRule automaticMediaDownloadsRule;
};

struct ClientSettings {
	explicit ClientSettings(Settings *settings = nullptr, const QVector<RosterItem> &rosterItems = {})
		: roster(transform(rosterItems,
			  [](const RosterItem &item) { return ClientRosterItemSettings(item); }))
	{
	}

	QVector<ClientRosterItemSettings> roster;
};

AccountMigrationManager::AccountMigrationManager(ClientWorker *clientWorker, QObject *parent)
	: QObject(parent),
	  m_worker(clientWorker),
	  m_manager(clientWorker->xmppClient()->findExtension<QXmppAccountMigrationManager>())
{
	connect(this, &AccountMigrationManager::migrationStateChanged, this, [this]() {
		Q_EMIT busyChanged(isBusy());
	});

	connect(clientWorker, &ClientWorker::loggedInWithNewCredentials, this, [this]() {
		if (migrationState() == MigrationState::Registering) {
			continueMigration();
		}
	});
}

AccountMigrationManager::~AccountMigrationManager() = default;

AccountMigrationManager::MigrationState AccountMigrationManager::migrationState() const
{
	return m_migrationData.has_value() ? m_migrationData->state : MigrationState::Idle;
}

void AccountMigrationManager::startMigration()
{
	if (migrationState() != MigrationState::Idle) {
		Q_EMIT errorOccurred(
			tr("Account could not be migrated: Another migration is already in progress"));
		return;
	}

	if (m_worker->xmppClient()->isAuthenticated()) {
		m_migrationData = MigrationData();

		if (QFile::exists(diskAccountFilePath())) {
			if (restoreAccountDataFromDisk(m_migrationData->account)) {
				m_migrationData->state = MigrationState::Registering;
			} else {
				Q_EMIT errorOccurred(tr("Account could not be migrated: Could not load exported "
										"account data"));
				m_migrationData.reset();
				return;
			}
		}

		continueMigration();
	} else {
		Q_EMIT errorOccurred(tr("Account could not be migrated: You need to be connected"));
	}
}

void AccountMigrationManager::continueMigration(const QVariant &userData)
{
	Q_UNUSED(userData)

	if (m_migrationData) {
		if (migrationState() == MigrationState::Finished) {
			m_migrationData.reset();
		} else {
			m_migrationData->advanceState();
		}

		Q_EMIT migrationStateChanged(migrationState());

		const auto fail = [this](const QString &message) {
			Q_EMIT errorOccurred(message);
			m_migrationData.reset();
			Q_EMIT migrationStateChanged(migrationState());
		};

		switch (migrationState()) {
		case MigrationState::Idle:
		case MigrationState::Started:
		case MigrationState::Registering:
		case MigrationState::Finished:
			break;

		case MigrationState::Exporting: {
			m_manager->exportData().then(this, [this, fail](auto &&result) {
				if (std::holds_alternative<QXmppError>(result)) {
					const auto error = std::get<QXmppError>(std::move(result));
					fail(error.description);
				} else {
					m_migrationData->account =
						std::move(std::get<QXmppExportData>(std::move(result)));
					continueMigration();
				}
			});

			break;
		}

		case MigrationState::Importing: {
			m_manager->importData(m_migrationData->account).then(this, [this, fail](auto &&result) {
				if (std::holds_alternative<QXmppError>(result)) {
					const auto error = std::get<QXmppError>(std::move(result));
					saveAccountDataToDisk(m_migrationData->account);
					fail(error.description);
				} else {
					const QString filePath = diskAccountFilePath();

					if (QFile::exists(filePath)) {
						QFile::remove(filePath);
					}

					continueMigration();
				}
			});

			break;
		}
		}
	}
}

void AccountMigrationManager::cancelMigration()
{
	if (migrationState() != MigrationState::Idle) {
		// Restore previous account settings and log in to the previous account.
		if (m_migrationData->state == MigrationState::Registering) {
			m_worker->registrationManager()->cancelRegistration();
		}

		m_migrationData.reset();
		Q_EMIT migrationStateChanged(migrationState());
	}
}

bool AccountMigrationManager::isBusy() const
{
	return migrationState() != MigrationState::Idle;
}

QString AccountMigrationManager::diskAccountFilePath() const
{
	const QString jidBare = m_worker->xmppClient()->configuration().jidBare();
	return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
		.absoluteFilePath(QStringLiteral("%1.xml").arg(jidBare));
}

bool AccountMigrationManager::saveAccountDataToDisk(const QXmppExportData &data)
{
	const QString filePath = diskAccountFilePath();
	QFile file(filePath);

	if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
		qDebug("Account could not be migrated: Could not open account file '%ls' for writing: %ls",
			qUtf16Printable(filePath),
			qUtf16Printable(file.errorString()));
		return false;
	}

	QXmlStreamWriter writer(&file);

	data.toXml(&writer);

	if (writer.hasError()) {
		file.remove();
		qDebug("Account could not be migrated: Could not write account data to file '%ls': %ls",
			qUtf16Printable(filePath),
			qUtf16Printable(file.errorString()));
		return false;
	}

	return true;
}

bool AccountMigrationManager::restoreAccountDataFromDisk(QXmppExportData &data)
{
	const QString filePath = diskAccountFilePath();
	QFile file(filePath);

	if (!file.open(QIODevice::ReadOnly)) {
		qDebug("Account could not be migrated: Could not open account file '%ls' for reading: %ls",
			qUtf16Printable(filePath),
			qUtf16Printable(file.errorString()));
		return false;
	}

	QDomDocument document;
	QString error;

	if (!document.setContent(&file, true, &error)) {
		qDebug("Account could not be migrated: Could not parse account data from '%ls': %ls", qUtf16Printable(filePath), qUtf16Printable(error));
		return false;
	}

	const auto result = QXmppExportData::fromDom(document.documentElement());

	if (std::holds_alternative<QXmppError>(result)) {
		qDebug("Account could not be migrated: Could not read account data from file '%ls': %ls",
			qUtf16Printable(filePath),
			qUtf16Printable(std::get<QXmppError>(result).description));
		return false;
	}

	data = std::move(std::get<QXmppExportData>(result));

	return true;
}
