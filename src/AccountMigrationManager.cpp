// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationManager.h"

#include "AccountManager.h"
#include "Algorithms.h"
#include "Encryption.h"
#include "FutureUtils.h"
#include "RegistrationManager.h"
#include "RosterDb.h"
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

#include <QXmppConfiguration.h>
#include <QXmppMovedManager.h>
#include <QXmppPubSubManager.h>

#define readDataElement(NAME)                                                                                                                                  \
    if (!XmlUtils::Reader::text(rootElement.firstChildElement(QStringLiteral(#NAME)), data.NAME)) {                                                            \
        return QXmppError{QStringLiteral("Invalid NAME element."), {}};                                                                                        \
    }

#define writeDataElement(NAME) XmlUtils::Writer::text(&writer, QStringLiteral(#NAME), NAME)

static constexpr QStringView s_kaidan_ns = u"kaidan";
static constexpr QStringView s_client_settings = u"client-settings";
static constexpr QStringView s_old_configuration = u"old-configuration";
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

        ClientRosterItemSettings data;

        if (const auto element = rootElement.firstChildElement(QStringLiteral("bareJid")); true) {
            data.bareJid = element.text();

            if (data.bareJid.isEmpty()) {
                return QXmppError{QStringLiteral("Missing required bareJid element."), {}};
            }
        }

        readDataElement(encryption);
        readDataElement(notificationRule);
        readDataElement(chatStateSendingEnabled);
        readDataElement(readMarkerSendingEnabled);
        readDataElement(pinningPosition);
        readDataElement(automaticMediaDownloadsRule);

        return data;
    }

    void toXml(QXmlStreamWriter &writer) const
    {
        writer.writeStartElement(s_item.toString());
        writeDataElement(bareJid);
        writeDataElement(encryption);
        writeDataElement(notificationRule);
        writeDataElement(chatStateSendingEnabled);
        writeDataElement(readMarkerSendingEnabled);
        writeDataElement(pinningPosition);
        writeDataElement(automaticMediaDownloadsRule);
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

struct ClientConfiguration {
    ClientConfiguration() = default;
    explicit ClientConfiguration(const Settings &settings)
    {
        authHost = settings.authHost();
        authPort = settings.authPort();
        authTlsErrorsIgnored = settings.authTlsErrorsIgnored();
        authTlsRequirement = settings.authTlsRequirement();
        authJid = settings.authJid();
        authPassword = settings.authPassword();
    }

    static std::variant<ClientConfiguration, QXmppError> fromDom(const QDomElement &rootElement)
    {
        Q_ASSERT(rootElement.tagName() == s_old_configuration);
        Q_ASSERT(rootElement.namespaceURI() == s_kaidan_ns);

        ClientConfiguration data;

        readDataElement(authHost);
        readDataElement(authPort);
        readDataElement(authTlsErrorsIgnored);
        readDataElement(authTlsRequirement);
        readDataElement(authJid);
        readDataElement(authPassword);

        return data;
    }

    void toXml(QXmlStreamWriter &writer) const
    {
        writer.writeStartElement(s_old_configuration.toString());
        writer.writeDefaultNamespace(s_kaidan_ns.toString());
        writeDataElement(authHost);
        writeDataElement(authPort);
        writeDataElement(authTlsErrorsIgnored);
        writeDataElement(authTlsRequirement);
        writeDataElement(authJid);
        writeDataElement(authPassword);
        writer.writeEndElement();
    }

    QXmppConfiguration toXmppConfiguration() const
    {
        QXmppConfiguration configuration;

        if (authHost) {
            configuration.setHost(*authHost);
        }

        if (authPort) {
            configuration.setPort(*authPort);
        }

        if (authTlsErrorsIgnored) {
            configuration.setIgnoreSslErrors(*authTlsErrorsIgnored);
        }

        if (authTlsRequirement) {
            configuration.setStreamSecurityMode(*authTlsRequirement);
        }

        if (authJid) {
            configuration.setJid(*authJid);
        }

        if (authPassword) {
            configuration.setPassword(*authPassword);
        }

        return configuration;
    }

    std::optional<QString> authHost;
    std::optional<quint16> authPort;
    std::optional<bool> authTlsErrorsIgnored;
    std::optional<QXmppConfiguration::StreamSecurityMode> authTlsRequirement;
    std::optional<QString> authJid;
    std::optional<QString> authPassword;
};

struct ClientSettings {
    ClientSettings() = default;
    explicit ClientSettings(Settings *settings, const QList<RosterItem> &rosterItems)
        : oldConfiguration(*settings)
        , roster(transform(rosterItems, [](const RosterItem &item) {
            return ClientRosterItemSettings(item);
        }))
    {
    }

    QList<QString> rosterContacts() const
    {
        return transform<QList<QString>>(roster, [](const ClientRosterItemSettings &item) {
            return item.bareJid;
        });
    }

    static std::variant<ClientSettings, QXmppError> fromDom(const QDomElement &rootElement)
    {
        Q_ASSERT(rootElement.tagName() == s_client_settings);
        Q_ASSERT(rootElement.namespaceURI() == s_qxmpp_export_ns);

        const auto oldConfigurationElement = rootElement.firstChildElement(s_old_configuration.toString());
        const auto rosterElement = rootElement.firstChildElement(s_roster.toString());
        ClientSettings settings;

        if (oldConfigurationElement.namespaceURI() == s_kaidan_ns) {
            const auto result = ClientConfiguration::fromDom(oldConfigurationElement);

            if (const auto error = std::get_if<QXmppError>(&result)) {
                return *error;
            }

            settings.oldConfiguration = std::get<ClientConfiguration>(result);
        }

        if (rosterElement.namespaceURI() == s_kaidan_ns) {
            settings.roster.reserve(rosterElement.childNodes().count());

            for (QDomNode node = rosterElement.firstChild(); !node.isNull(); node = node.nextSibling()) {
                if (!node.isElement()) {
                    continue;
                }

                const auto element = node.toElement();

                if (element.tagName() == s_item) {
                    const auto result = ClientRosterItemSettings::fromDom(element);

                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        return *error;
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

        // Old configuration
        oldConfiguration.toXml(writer);

        // Roster items
        writer.writeStartElement(s_roster.toString());
        writer.writeDefaultNamespace(s_kaidan_ns.toString());
        for (const auto &entry : roster) {
            entry.toXml(writer);
        }
        writer.writeEndElement();

        writer.writeEndElement();
    }

    ClientConfiguration oldConfiguration;
    QList<ClientRosterItemSettings> roster;
};

class PublishMovedStatementClient : public QXmppClient
{
    Q_OBJECT

public:
    explicit PublishMovedStatementClient(QObject *parent = nullptr)
        : QXmppClient(parent)
        , m_pubSubManager(addNewExtension<QXmppPubSubManager>())
        , m_movedManager(addNewExtension<QXmppMovedManager>())
    {
        connect(this, &QXmppClient::stateChanged, this, &PublishMovedStatementClient::onStateChanged);
        connect(this, &QXmppClient::errorOccurred, this, &PublishMovedStatementClient::onErrorOccurred);
        connect(this, &PublishMovedStatementClient::finished, this, &QObject::deleteLater);
        connect(this, &PublishMovedStatementClient::finished, this, [](const QXmppClient::EmptyResult &result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                qDebug("%s: Publishing failed - %ls", Q_FUNC_INFO, qUtf16Printable(error->description));
            } else {
                qDebug("%s: Published successfully", Q_FUNC_INFO);
            }
        });
    }

    ~PublishMovedStatementClient() override = default;

    void publishStatement(const QXmppConfiguration &configuration, const QString &newBareJid)
    {
        Q_ASSERT(!newBareJid.isEmpty());
        m_newBareJid = newBareJid;
        connectToServer(configuration);
    }

    Q_SIGNAL void finished(const QXmppClient::EmptyResult &result);

private:
    void onStateChanged(QXmppClient::State state)
    {
        switch (state) {
        case QXmppClient::DisconnectedState:
            onDisconnected();
            break;
        case QXmppClient::ConnectingState:
            break;
        case QXmppClient::ConnectedState:
            onConnected();
            break;
        }
    }

    void onErrorOccurred(const QXmppError &error)
    {
        qDebug("%s: %ls", Q_FUNC_INFO, qUtf16Printable(error.description));

        if (isConnected()) {
            disconnectFromServer();
        } else {
            onDisconnected();
        }

        Q_EMIT finished(error);
    }

    void onConnected()
    {
        m_movedManager->publishStatement(m_newBareJid).then(this, [this](auto &&result) {
            disconnectFromServer();
            Q_EMIT finished(std::move(std::get<QXmpp::Success>(std::move(result))));
        });
    }

    void onDisconnected()
    {
    }

private:
    QXmppPubSubManager *const m_pubSubManager;
    QXmppMovedManager *const m_movedManager;
    QString m_newBareJid;
};

AccountMigrationManager::AccountMigrationManager(ClientWorker *clientWorker, QObject *parent)
    : QObject(parent)
    , m_worker(clientWorker)
    , m_migrationManager(clientWorker->xmppClient()->findExtension<QXmppAccountMigrationManager>())
    , m_movedManager(clientWorker->xmppClient()->findExtension<QXmppMovedManager>())
{
    // Those are mandatory
    Q_ASSERT(m_migrationManager);
    Q_ASSERT(m_movedManager);

    constexpr const auto parseData = &ClientSettings::fromDom;
    const auto serializeData = [](const ClientSettings &data, QXmlStreamWriter &writer) {
        data.toXml(writer);
    };

    QXmppExportData::registerExtension<ClientSettings, parseData, serializeData>(s_client_settings, s_qxmpp_export_ns);

    connect(this, &AccountMigrationManager::migrationStateChanged, this, [this]() {
        Q_EMIT busyChanged(isBusy());
    });

    connect(m_migrationManager, &QXmppAccountMigrationManager::errorOccurred, this, [this](const QXmppError &error) {
        Q_EMIT errorOccurred(error.description);
    });

    connect(clientWorker, &ClientWorker::loggedInWithNewCredentials, this, [this]() {
        if (migrationState() == MigrationState::ChoosingNewAccount) {
            continueMigration();
        }
    });
}

AccountMigrationManager::~AccountMigrationManager() = default;

AccountMigrationManager::MigrationState AccountMigrationManager::migrationState() const
{
    return m_migrationData ? m_migrationData->state : MigrationState::Idle;
}

void AccountMigrationManager::startMigration()
{
    if (migrationState() != MigrationState::Idle) {
        Q_EMIT errorOccurred(tr("Account could not be migrated: Another migration is already in progress"));
        return;
    }

    if (m_worker->xmppClient()->isAuthenticated()) {
        m_migrationData = MigrationData();

        if (QFile::exists(diskAccountFilePath())) {
            if (restoreAccountDataFromDisk(m_migrationData->account)) {
                m_migrationData->state = MigrationState::ChoosingNewAccount;
            } else {
                Q_EMIT errorOccurred(
                    tr("Account could not be migrated: Could not load exported "
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
            m_migrationManager->exportData().then(this, [this, fail](auto &&result) {
                if (const auto error = std::get_if<QXmppError>(&result)) {
                    fail(error->description);
                } else {
                    m_migrationData->account = std::move(std::get<QXmppExportData>(std::move(result)));

                    exportClientSettingsTask().then(this, [this, fail](auto &&result) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            fail(error->description);
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
            m_migrationManager->importData(m_migrationData->account).then(this, [this, fail](auto &&result) {
                if (const auto error = std::get_if<QXmppError>(&result)) {
                    saveAccountDataToDisk(m_migrationData->account);
                    fail(error->description);
                } else {
                    const QString filePath = diskAccountFilePath();

                    if (QFile::exists(filePath)) {
                        QFile::remove(filePath);
                    }

                    const auto clientSettings = m_migrationData->account.extension<ClientSettings>();

                    if (clientSettings) {
                        importClientSettingsTask(*clientSettings)
                            .then(this,
                                  [this,
                                   fail,
                                   configuration = clientSettings->oldConfiguration.toXmppConfiguration(),
                                   contacts = clientSettings->rosterContacts()](auto &&result) {
                                      if (const auto error = std::get_if<QXmppError>(&result)) {
                                          fail(error->description);
                                      } else {
                                          publishMovedStatement(configuration, m_migrationData->account.accountJid())
                                              .then(this, [this, fail, contacts](auto &&result) {
                                                  if (const auto error = std::get_if<QXmppError>(&result)) {
                                                      fail(error->description);
                                                  } else {
                                                      notifyContacts(contacts, m_migrationData->account.accountJid()).then(this, [&](auto &&result) {
                                                          if (const auto error = std::get_if<QXmppError>(&result)) {
                                                              fail(error->description);
                                                          } else {
                                                              continueMigration();
                                                          }
                                                      });
                                                  }
                                              });
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
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).absoluteFilePath(QStringLiteral("%1.xml").arg(jidBare));
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

    if (const auto error = std::get_if<QXmppError>(&result)) {
        qDebug("Account could not be migrated: Could not read account data from file '%ls': %ls",
               qUtf16Printable(filePath),
               qUtf16Printable(error->description));
        return false;
    }

    data = std::move(std::get<QXmppExportData>(result));

    return true;
}

QXmppTask<AccountMigrationManager::ImportResult> AccountMigrationManager::importClientSettingsTask(const ClientSettings &settings)
{
    return runAsyncTask(this, Kaidan::instance(), [settings]() -> ImportResult {
        for (const ClientRosterItemSettings &itemSettings : settings.roster) {
            RosterDb::instance()->updateItem(itemSettings.bareJid, [itemSettings](RosterItem &item) {
                item.encryption = itemSettings.encryption.value_or(item.encryption);
                item.notificationRule = itemSettings.notificationRule.value_or(item.notificationRule);
                item.chatStateSendingEnabled = itemSettings.chatStateSendingEnabled.value_or(item.chatStateSendingEnabled);
                item.readMarkerSendingEnabled = itemSettings.readMarkerSendingEnabled.value_or(item.readMarkerSendingEnabled);
                item.pinningPosition = itemSettings.pinningPosition.value_or(item.pinningPosition);
                item.automaticMediaDownloadsRule = itemSettings.automaticMediaDownloadsRule.value_or(item.automaticMediaDownloadsRule);
            });
        }

        return QXmpp::Success{};
    });
}

QXmppTask<AccountMigrationManager::ExportResult> AccountMigrationManager::exportClientSettingsTask()
{
    return runAsyncTask(this, Kaidan::instance(), [settings = m_worker->caches()->settings]() -> ExportResult {
        return ClientSettings(settings, RosterModel::instance()->items());
    });
}

QXmppTask<QXmppAccountMigrationManager::Result<>> AccountMigrationManager::publishMovedStatement(const QXmppConfiguration &configuration,
                                                                                                 const QString &newBareJid)
{
    auto client = new PublishMovedStatementClient(this);
    QXmppPromise<QXmppAccountMigrationManager::Result<>> promise;
    auto task = promise.task();

    connect(client, &PublishMovedStatementClient::finished, this, [promise](const auto &result) mutable {
        promise.finish(result);
    });

    client->publishStatement(configuration, newBareJid);

    return task;
}

QXmppTask<QXmppAccountMigrationManager::Result<>> AccountMigrationManager::notifyContacts(const QList<QString> &contactsBareJids, const QString &oldBareJid)
{
    if (contactsBareJids.isEmpty()) {
        return makeReadyTask<QXmppAccountMigrationManager::Result<>>({});
    }

    QXmppPromise<QXmppAccountMigrationManager::Result<>> promise;
    auto counter = std::make_shared<int>(contactsBareJids.size());

    for (const QString &contactBareJid : contactsBareJids) {
        m_movedManager->notifyContact(contactBareJid, oldBareJid, false).then(this, [promise, counter](auto &&result) mutable {
            if (promise.task().isFinished()) {
                return;
            }

            if (const auto error = std::get_if<QXmppError>(&result)) {
                return promise.finish(*error);
            }

            if ((--(*counter)) == 0) {
                promise.finish(QXmpp::Success());
            }
        });
    }

    return promise.task();
}

#include "AccountMigrationManager.moc"

#include "moc_AccountMigrationManager.cpp"
