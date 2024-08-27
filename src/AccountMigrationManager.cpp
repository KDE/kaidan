// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationManager.h"

#include "AccountManager.h"
#include "Algorithms.h"
#include "Encryption.h"
#include "FutureUtils.h"
#include "RegistrationManager.h"
#include "RosterItem.h"
#include "RosterModel.h"
#include "Settings.h"
#include "XmlUtils.h"

#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>

#include <QXmppAccountMigrationManager.h>

#define readRosterSettingsElement(NAME) \
	if (!XmlUtils::Reader::text(rootElement.firstChildElement(QStringLiteral(#NAME)), settings.NAME)) { \
		return QXmppError { QStringLiteral("Invalid NAME element."), {} }; \
	}

#define writeRosterSettingsElement(NAME) \
	XmlUtils::Writer::text(&writer, QStringLiteral(#NAME), NAME)

static constexpr QStringView s_kaidan_ns = u"kaidan";
static constexpr QStringView s_client_settings = u"client-settings";
static constexpr QStringView s_roster = u"roster";
static constexpr QStringView s_item = u"item";
static constexpr QStringView s_qxmpp_export_ns = u"org.qxmpp.export";

struct ClientRosterItemSettings {
	ClientRosterItemSettings() = default;
	explicit ClientRosterItemSettings(const RosterItem &item)
	{
		bareJid = item.jid;
		encryption = item.encryption;
		notificationRule = item.notificationRule;
		chatStateSendingEnabled = item.chatStateSendingEnabled;
		readMarkerSendingEnabled = item.readMarkerSendingEnabled;
		pinningPosition = item.pinningPosition;
		automaticMediaDownloadsRule = item.automaticMediaDownloadsRule;
	}

	static std::variant<ClientRosterItemSettings, QXmppError> fromDom(const QDomElement &rootElement)
	{
		Q_ASSERT(rootElement.tagName() == s_item);
		Q_ASSERT(rootElement.namespaceURI() == s_kaidan_ns);

		ClientRosterItemSettings settings;

		if (const auto element = rootElement.firstChildElement(QStringLiteral("bareJid")); true) {
			settings.bareJid = element.text();

			if (settings.bareJid.isEmpty()) {
				return QXmppError {
					QStringLiteral("Missing required bareJid element."), {}
				};
			}
		}

		readRosterSettingsElement(encryption);
		readRosterSettingsElement(notificationRule);
		readRosterSettingsElement(chatStateSendingEnabled);
		readRosterSettingsElement(readMarkerSendingEnabled);
		readRosterSettingsElement(pinningPosition);
		readRosterSettingsElement(automaticMediaDownloadsRule);

		return settings;
	}

	void toXml(QXmlStreamWriter &writer) const
	{
		writer.writeStartElement(s_item.toString());
		writeRosterSettingsElement(bareJid);
		writeRosterSettingsElement(encryption);
		writeRosterSettingsElement(notificationRule);
		writeRosterSettingsElement(chatStateSendingEnabled);
		writeRosterSettingsElement(readMarkerSendingEnabled);
		writeRosterSettingsElement(pinningPosition);
		writeRosterSettingsElement(automaticMediaDownloadsRule);
		writer.writeEndElement();
	}

	QString bareJid;
	std::optional<Encryption::Enum> encryption;
	std::optional<RosterItem::NotificationRule> notificationRule;
	std::optional<bool> chatStateSendingEnabled;
	std::optional<bool> readMarkerSendingEnabled;
	std::optional<int> pinningPosition;
	std::optional<RosterItem::AutomaticMediaDownloadsRule> automaticMediaDownloadsRule;
};

struct ClientSettings {
	ClientSettings() = default;
	explicit ClientSettings(Settings *settings, const QVector<RosterItem> &rosterItems)
		: roster(transform(rosterItems,
			  [](const RosterItem &item) { return ClientRosterItemSettings(item); }))
	{
		Q_UNUSED(settings)
	}

	static std::variant<ClientSettings, QXmppError> fromDom(const QDomElement &rootElement)
	{
		Q_ASSERT(rootElement.tagName() == s_client_settings);
		Q_ASSERT(rootElement.namespaceURI() == s_qxmpp_export_ns);

		const auto rosterElement = rootElement.firstChildElement(s_roster.toString());
		ClientSettings settings;

		if (rosterElement.namespaceURI() == s_kaidan_ns) {
			settings.roster.reserve(rosterElement.childNodes().count());

			for (QDomNode node = rosterElement.firstChild(); !node.isNull();
				node = node.nextSibling()) {
				if (!node.isElement()) {
					continue;
				}

				const auto element = node.toElement();

				if (element.tagName() == s_item) {
					const auto result = ClientRosterItemSettings::fromDom(element);

					if (std::holds_alternative<QXmppError>(result)) {
						return std::get<QXmppError>(result);
					}

					settings.roster.append(std::get<ClientRosterItemSettings>(result));
				}
			}
		}

		return settings;
	}

	void toXml(QXmlStreamWriter &writer) const
	{
		writer.writeStartElement(s_client_settings.toString());
		writer.writeStartElement(s_roster.toString());
		writer.writeDefaultNamespace(s_kaidan_ns.toString());
		for (const auto &entry : roster) {
			entry.toXml(writer);
		}
		writer.writeEndElement();
		writer.writeEndElement();
	}

	QVector<ClientRosterItemSettings> roster;
};

AccountMigrationManager::AccountMigrationManager(ClientWorker *clientWorker, QObject *parent)
	: QObject(parent),
	  m_worker(clientWorker),
	  m_manager(clientWorker->xmppClient()->findExtension<QXmppAccountMigrationManager>())
{
	constexpr const auto parseData = &ClientSettings::fromDom;
	const auto serializeData = [](const ClientSettings &data, QXmlStreamWriter &writer) {
		data.toXml(writer);
	};

	QXmppExportData::registerExtension<ClientSettings, parseData, serializeData>(s_client_settings, s_qxmpp_export_ns);

	connect(this, &AccountMigrationManager::migrationStateChanged, this, [this]() {
		Q_EMIT busyChanged(isBusy());
	});

	connect(clientWorker, &ClientWorker::loggedInWithNewCredentials, this, [this]() {
		if (migrationState() == MigrationState::ChoosingNewAccount) {
			continueMigration();
		}
	});
}

AccountMigrationManager::~AccountMigrationManager()
{
	m_manager->unregisterExportData<ClientSettings>();
}

AccountMigrationManager::MigrationState AccountMigrationManager::migrationState() const
{
	return m_migrationData ? m_migrationData->state : MigrationState::Idle;
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
				m_migrationData->state = MigrationState::ChoosingNewAccount;
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
		case MigrationState::ChoosingNewAccount:
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

					exportClientSettingsTask().then(this, [this, fail](auto &&result) {
						if (std::holds_alternative<QXmppError>(result)) {
							const auto error = std::get<QXmppError>(std::move(result));
							fail(error.description);
						} else {
							m_migrationData->account.setExtension(std::move(std::get<ClientSettings>(std::move(result))));

							continueMigration();
						}
					});
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

					const auto clientSettings = m_migrationData->account.extension<ClientSettings>();

					if (clientSettings) {
						importClientSettingsTask(*clientSettings).then(this, [this, fail](auto &&result) {
							if (std::holds_alternative<QXmppError>(result)) {
								const auto error = std::get<QXmppError>(std::move(result));
								fail(error.description);
							} else {
								continueMigration();
							}
						});
					} else {
						continueMigration();
					}
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
		// Restore the previous account.
		if (m_migrationData->state == MigrationState::ChoosingNewAccount) {
			const auto accountManager = AccountManager::instance();

			// If the stored JID differs from the cached one, reconnect with the stored one.
			if (accountManager->jid() != m_worker->caches()->settings->authJid()) {
				// Disconnect from any server that the client is connecting to because of a login
				// attempt.
				if (m_worker->xmppClient()->state() == QXmppClient::ConnectingState) {
					m_worker->xmppClient()->disconnectFromServer();
				}

				// Resetting the cached JID is needed to load the stored JID via
				// AccountManager::loadConnectionData().
				accountManager->setJid({});

				if (accountManager->loadConnectionData()) {
					m_worker->logIn();
				}
			}
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

QXmppTask<AccountMigrationManager::ImportResult> AccountMigrationManager::importClientSettingsTask(
	const ClientSettings &settings)
{
	return runAsyncTask(this, Kaidan::instance(), [settings]() -> ImportResult {
		auto model = RosterModel::instance();

		for (const ClientRosterItemSettings &itemSettings : settings.roster) {
			model->updateItem(itemSettings.bareJid, [itemSettings](RosterItem &item) {
				item.encryption = itemSettings.encryption.value_or(item.encryption);
				item.notificationRule = itemSettings.notificationRule.value_or(item.notificationRule);
				item.chatStateSendingEnabled = itemSettings.chatStateSendingEnabled.value_or(item.chatStateSendingEnabled);
				item.readMarkerSendingEnabled = itemSettings.readMarkerSendingEnabled.value_or(item.readMarkerSendingEnabled);
				item.pinningPosition = itemSettings.pinningPosition.value_or(item.pinningPosition);
				item.automaticMediaDownloadsRule = itemSettings.automaticMediaDownloadsRule.value_or(item.automaticMediaDownloadsRule);
			});
		}

		return QXmpp::Success {};
	});
}

QXmppTask<AccountMigrationManager::ExportResult> AccountMigrationManager::exportClientSettingsTask()
{
	return runAsyncTask(this, Kaidan::instance(), [settings = m_worker->caches()->settings]() -> ExportResult {
		return ClientSettings(settings, RosterModel::instance()->items());
	});
}
