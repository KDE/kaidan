// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountMigrationController.h"

// Qt
#include <QDir>
#include <QDomDocument>
#include <QFile>
#include <QSaveFile>
#include <QStandardPaths>
#include <QUrl>
// QXmpp
#include <QXmppConfiguration.h>
#include <QXmppMovedManager.h>
#include <QXmppPubSubManager.h>
// Kaidan
#include "AccountController.h"
#include "AccountDb.h"
#include "Algorithms.h"
#include "Encryption.h"
#include "FutureUtils.h"
#include "KaidanCoreLog.h"
#include "RosterDb.h"
#include "RosterItem.h"
#include "RosterModel.h"
#include "XmlUtils.h"

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
        jid = item.jid;
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

        if (const auto element = rootElement.firstChildElement(QStringLiteral("jid")); true) {
            data.jid = element.text();

            if (data.jid.isEmpty()) {
                return QXmppError{QStringLiteral("Missing required 'jid' element."), {}};
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
        writeDataElement(jid);
        writeDataElement(encryption);
        writeDataElement(notificationRule);
        writeDataElement(chatStateSendingEnabled);
        writeDataElement(readMarkerSendingEnabled);
        writeDataElement(pinningPosition);
        writeDataElement(automaticMediaDownloadsRule);
        writer.writeEndElement();
    }

    QString jid;
    std::optional<Encryption::Enum> encryption;
    std::optional<RosterItem::NotificationRule> notificationRule;
    std::optional<bool> chatStateSendingEnabled;
    std::optional<bool> readMarkerSendingEnabled;
    std::optional<int> pinningPosition;
    std::optional<RosterItem::AutomaticMediaDownloadsRule> automaticMediaDownloadsRule;
};

struct ClientConfiguration {
    ClientConfiguration() = default;
    explicit ClientConfiguration(const Account &account)
    {
        jid = account.jid;
        password = account.password;
        host = account.host;
        port = account.port;
        tlsErrorsIgnored = account.tlsErrorsIgnored;
        tlsRequirement = account.tlsRequirement;
        passwordVisibility = account.passwordVisibility;
        encryption = account.encryption;
        automaticMediaDownloadsRule = account.automaticMediaDownloadsRule;
        name = account.name;
        contactNotificationRule = account.contactNotificationRule;
        groupChatNotificationRule = account.groupChatNotificationRule;
        geoLocationMapPreviewEnabled = account.geoLocationMapPreviewEnabled;
        geoLocationMapService = account.geoLocationMapService;
    }

    static std::variant<ClientConfiguration, QXmppError> fromDom(const QDomElement &rootElement)
    {
        Q_ASSERT(rootElement.tagName() == s_old_configuration);
        Q_ASSERT(rootElement.namespaceURI() == s_kaidan_ns);

        ClientConfiguration data;

        readDataElement(jid);
        readDataElement(password);
        readDataElement(host);
        readDataElement(port);
        readDataElement(tlsErrorsIgnored);
        readDataElement(tlsRequirement);
        readDataElement(passwordVisibility);
        readDataElement(encryption);
        readDataElement(automaticMediaDownloadsRule);
        readDataElement(name);
        readDataElement(contactNotificationRule);
        readDataElement(groupChatNotificationRule);
        readDataElement(geoLocationMapPreviewEnabled);
        readDataElement(geoLocationMapService);

        return data;
    }

    void toXml(QXmlStreamWriter &writer) const
    {
        writer.writeStartElement(s_old_configuration.toString());
        writer.writeDefaultNamespace(s_kaidan_ns.toString());
        writeDataElement(jid);
        writeDataElement(password);
        writeDataElement(host);
        writeDataElement(port);
        writeDataElement(tlsErrorsIgnored);
        writeDataElement(tlsRequirement);
        writeDataElement(passwordVisibility);
        writeDataElement(encryption);
        writeDataElement(automaticMediaDownloadsRule);
        writeDataElement(name);
        writeDataElement(contactNotificationRule);
        writeDataElement(groupChatNotificationRule);
        writeDataElement(geoLocationMapPreviewEnabled);
        writeDataElement(geoLocationMapService);
        writer.writeEndElement();
    }

    QXmppConfiguration toXmppConfiguration() const
    {
        QXmppConfiguration configuration;

        if (jid) {
            configuration.setJid(*jid);
        }

        if (password) {
            configuration.setPassword(*password);
        }

        if (host) {
            configuration.setHost(*host);
        }

        if (port) {
            configuration.setPort(*port);
        }

        if (tlsErrorsIgnored) {
            configuration.setIgnoreSslErrors(*tlsErrorsIgnored);
        }

        if (tlsRequirement) {
            configuration.setStreamSecurityMode(*tlsRequirement);
        }

        return configuration;
    }

    std::optional<QString> jid;
    std::optional<QString> password;
    std::optional<QString> host;
    std::optional<quint16> port;
    std::optional<bool> tlsErrorsIgnored;
    std::optional<QXmppConfiguration::StreamSecurityMode> tlsRequirement;
    std::optional<Kaidan::PasswordVisibility> passwordVisibility;
    std::optional<Encryption::Enum> encryption;
    std::optional<Account::AutomaticMediaDownloadsRule> automaticMediaDownloadsRule;
    std::optional<QString> name;
    std::optional<Account::ContactNotificationRule> contactNotificationRule;
    std::optional<Account::GroupChatNotificationRule> groupChatNotificationRule;
    std::optional<bool> geoLocationMapPreviewEnabled;
    std::optional<Account::GeoLocationMapService> geoLocationMapService;
};

struct ClientSettings {
    ClientSettings() = default;
    explicit ClientSettings(const Account &account, const QList<RosterItem> &rosterItems)
        : oldConfiguration(account)
        , roster(transform(rosterItems, [](const RosterItem &item) {
            return ClientRosterItemSettings(item);
        }))
    {
    }

    QList<QString> rosterContacts() const
    {
        return transform<QList<QString>>(roster, [](const ClientRosterItemSettings &item) {
            return item.jid;
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
            settings.roster.reserve(rosterElement.childNodes().size());

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
                qCDebug(KAIDAN_CORE_LOG, "%s: Publishing failed - %ls", Q_FUNC_INFO, qUtf16Printable(error->description));
            } else {
                qCDebug(KAIDAN_CORE_LOG, "%s: Published successfully", Q_FUNC_INFO);
            }
        });
    }

    ~PublishMovedStatementClient() override = default;

    void publishStatement(const QXmppConfiguration &configuration, const QString &newJid)
    {
        Q_ASSERT(!newJid.isEmpty());
        m_newJid = newJid;
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
        qCDebug(KAIDAN_CORE_LOG, "%s: %ls", Q_FUNC_INFO, qUtf16Printable(error.description));

        if (isConnected()) {
            disconnectFromServer();
        } else {
            onDisconnected();
        }

        Q_EMIT finished(error);
    }

    void onConnected()
    {
        m_movedManager->publishStatement(m_newJid).then(this, [this](auto &&result) {
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
    QString m_newJid;
};

AccountMigrationController::AccountMigrationController(QObject *parent)
    : QObject(parent)
    , m_clientWorker(Kaidan::instance()->client())
    , m_client(m_clientWorker->xmppClient())
    , m_migrationManager(m_clientWorker->accountMigrationManager())
    , m_movedManager(m_clientWorker->movedManager())
{
    constexpr const auto parseData = &ClientSettings::fromDom;
    const auto serializeData = [](const ClientSettings &data, QXmlStreamWriter &writer) {
        data.toXml(writer);
    };

    QXmppExportData::registerExtension<ClientSettings, parseData, serializeData>(s_client_settings, s_qxmpp_export_ns);

    connect(this, &AccountMigrationController::migrationStateChanged, this, [this]() {
        Q_EMIT busyChanged();

        switch (migrationState()) {
        case MigrationState::Idle:
        case MigrationState::Exporting:
        case MigrationState::Importing:
            break;
        case MigrationState::Started:
        case MigrationState::Finished:
            continueMigration();
            break;
        case MigrationState::ChoosingNewAccount:
            Q_EMIT Kaidan::instance()->openStartPageRequested();
            break;
        }
    });

    connect(m_migrationManager, &QXmppAccountMigrationManager::errorOccurred, this, [this](const QXmppError &error) {
        handleError(error.description);
    });

    connect(m_clientWorker, &ClientWorker::loggedInWithNewCredentials, this, [this]() {
        if (migrationState() == MigrationState::ChoosingNewAccount) {
            continueMigration();
        }
    });
}

AccountMigrationController::~AccountMigrationController() = default;

AccountMigrationController::MigrationState AccountMigrationController::migrationState() const
{
    return m_migrationData ? m_migrationData->state : MigrationState::Idle;
}

void AccountMigrationController::startMigration()
{
    if (migrationState() != MigrationState::Idle) {
        handleError(tr("Account could not be migrated: Another migration is already in progress"));
        return;
    }

    runOnThread(
        m_clientWorker,
        [this]() {
            return m_client->isAuthenticated();
        },
        this,
        [this](bool isAuthenticated) {
            if (isAuthenticated) {
                m_migrationData = MigrationData();

                diskAccountFilePath().then(this, [this](const QString &accountFilePath) {
                    if (QFile::exists(accountFilePath)) {
                        if (restoreAccountDataFromDisk(accountFilePath, m_migrationData->account)) {
                            m_migrationData->state = MigrationState::ChoosingNewAccount;
                        } else {
                            handleError(
                                tr("Account could not be migrated: Could not load exported "
                                   "account data"));
                            m_migrationData.reset();
                            return;
                        }
                    }

                    continueMigration();
                });
            } else {
                handleError(tr("Account could not be migrated: You need to be connected"));
            }
        });
}

void AccountMigrationController::cancelMigration()
{
    if (migrationState() != MigrationState::Idle) {
        // Restore the previous account.
        if (m_migrationData->state == MigrationState::ChoosingNewAccount) {
            const auto accountController = AccountController::instance();

            // If the stored JID differs from the cached one, reconnect with the stored one.
            if (accountController->hasNewAccount()) {
                // Disconnect from any server that the client is connecting to because of a login
                // attempt.
                if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnecting) {
                    runOnThread(m_client, [this]() {
                        m_client->disconnectFromServer();
                    });
                }

                // Resetting the cached JID is needed to load the stored JID via
                // AccountController::loadConnectionData().
                accountController->resetNewAccount();

                if (accountController->hasEnoughCredentialsForLogin()) {
                    runOnThread(m_clientWorker, [this]() {
                        m_clientWorker->logIn();
                    });
                }
            }
        }

        m_migrationData.reset();
        Q_EMIT migrationStateChanged();
    }
}

bool AccountMigrationController::busy() const
{
    return migrationState() != MigrationState::Idle;
}

void AccountMigrationController::continueMigration(const QVariant &userData)
{
    Q_UNUSED(userData)

    if (m_migrationData) {
        if (migrationState() == MigrationState::Finished) {
            m_migrationData.reset();
        } else {
            m_migrationData->advanceState();
        }

        Q_EMIT migrationStateChanged();

        const auto fail = [this](const QString &error) {
            handleError(error);
            m_migrationData.reset();
            Q_EMIT migrationStateChanged();
        };

        switch (migrationState()) {
        case MigrationState::Idle:
        case MigrationState::Started:
        case MigrationState::ChoosingNewAccount:
        case MigrationState::Finished:
            break;

        case MigrationState::Exporting: {
            callRemoteTask(
                m_migrationManager,
                [this]() {
                    return std::pair{m_migrationManager->exportData(), this};
                },
                this,
                [this, fail](QXmppAccountMigrationManager::Result<QXmppExportData> &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        fail(error->description);
                    } else {
                        m_migrationData->account = std::move(std::get<QXmppExportData>(std::move(result)));
                        m_migrationData->account.setExtension(exportClientSettings());
                        continueMigration();
                    }
                });
            break;
        }

        case MigrationState::Importing: {
            callRemoteTask(
                m_migrationManager,
                [this, accountData = m_migrationData->account]() {
                    return std::pair{m_migrationManager->importData(accountData), this};
                },
                this,
                [this, fail](auto &&result) {
                    diskAccountFilePath().then(this, [this, fail, result](const QString &accountFilePath) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            saveAccountDataToDisk(accountFilePath, m_migrationData->account);
                            fail(error->description);
                        } else {
                            if (QFile::exists(accountFilePath)) {
                                QFile::remove(accountFilePath);
                            }

                            const auto clientSettings = m_migrationData->account.extension<ClientSettings>();

                            if (clientSettings) {
                                importClientSettings(*clientSettings)
                                    .then(this,
                                          [this,
                                           fail,
                                           newJid = AccountController::instance()->account().jid,
                                           oldJid = m_migrationData->account.accountJid(),
                                           configuration = clientSettings->oldConfiguration.toXmppConfiguration(),
                                           contacts = clientSettings->rosterContacts()]() {
                                              publishMovedStatement(configuration, newJid).then(this, [this, fail, contacts, oldJid](auto &&result) {
                                                  if (const auto error = std::get_if<QXmppError>(&result)) {
                                                      fail(error->description);
                                                  } else {
                                                      notifyContacts(contacts, oldJid).then(this, [this, fail](auto &&result) {
                                                          if (const auto error = std::get_if<QXmppError>(&result)) {
                                                              fail(error->description);
                                                          } else {
                                                              continueMigration();
                                                          }
                                                      });
                                                  }
                                              });
                                          });
                            } else {
                                continueMigration();
                            }
                        }
                    });
                });
            break;
        }
        }
    }
}

void AccountMigrationController::handleError(const QString &error)
{
    switch (migrationState()) {
    case MigrationState::Idle:
    case MigrationState::Exporting:
    case MigrationState::ChoosingNewAccount:
    case MigrationState::Importing:
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(error);
        break;
    case MigrationState::Started:
    case MigrationState::Finished:
        break;
    }
}

QFuture<QString> AccountMigrationController::diskAccountFilePath() const
{
    return runAsync(m_clientWorker, [this]() {
        const QString accountJid = m_client->configuration().jidBare();
        return QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)).absoluteFilePath(QStringLiteral("%1.xml").arg(accountJid));
    });
}

bool AccountMigrationController::saveAccountDataToDisk(const QString &filePath, const QXmppExportData &data)
{
    QFile file(filePath);

    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not open account file '%ls' for writing: %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    QXmlStreamWriter writer(&file);

    data.toXml(&writer);

    if (writer.hasError()) {
        file.remove();
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not write account data to file '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    return true;
}

bool AccountMigrationController::restoreAccountDataFromDisk(const QString &filePath, QXmppExportData &data)
{
    QFile file(filePath);

    if (!file.open(QIODevice::ReadOnly)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not open account file '%ls' for reading: %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(file.errorString()));
        return false;
    }

    QDomDocument document;

    if (const auto res = document.setContent(&file, QDomDocument::ParseOption::UseNamespaceProcessing); !res) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not parse account data from '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(res.errorMessage));
        return false;
    }

    const auto result = QXmppExportData::fromDom(document.documentElement());

    if (const auto error = std::get_if<QXmppError>(&result)) {
        qCDebug(KAIDAN_CORE_LOG,
                "Account could not be migrated: Could not read account data from file '%ls': %ls",
                qUtf16Printable(filePath),
                qUtf16Printable(error->description));
        return false;
    }

    data = std::move(std::get<QXmppExportData>(result));

    return true;
}

QFuture<void> AccountMigrationController::importClientSettings(const ClientSettings &settings)
{
    return AccountDb::instance()
        ->updateAccount(AccountController::instance()->account().jid,
                        [settings = settings.oldConfiguration](Account &account) {
                            if (settings.tlsErrorsIgnored) {
                                account.tlsErrorsIgnored = *settings.tlsErrorsIgnored;
                            }

                            if (settings.tlsRequirement) {
                                account.tlsRequirement = *settings.tlsRequirement;
                            }

                            if (settings.passwordVisibility) {
                                account.passwordVisibility = *settings.passwordVisibility;
                            }

                            if (settings.encryption) {
                                account.encryption = *settings.encryption;
                            }

                            if (settings.automaticMediaDownloadsRule) {
                                account.automaticMediaDownloadsRule = *settings.automaticMediaDownloadsRule;
                            }

                            if (settings.name) {
                                account.name = *settings.name;
                            }

                            if (settings.contactNotificationRule) {
                                account.contactNotificationRule = *settings.contactNotificationRule;
                            }

                            if (settings.groupChatNotificationRule) {
                                account.groupChatNotificationRule = *settings.groupChatNotificationRule;
                            }

                            if (settings.geoLocationMapPreviewEnabled) {
                                account.geoLocationMapPreviewEnabled = *settings.geoLocationMapPreviewEnabled;
                            }

                            if (settings.geoLocationMapService) {
                                account.geoLocationMapService = *settings.geoLocationMapService;
                            }
                        })
        .then(this, [settings]() {
            for (const ClientRosterItemSettings &itemSettings : settings.roster) {
                RosterDb::instance()->updateItem(itemSettings.jid, [itemSettings](RosterItem &item) {
                    item.encryption = itemSettings.encryption.value_or(item.encryption);
                    item.notificationRule = itemSettings.notificationRule.value_or(item.notificationRule);
                    item.chatStateSendingEnabled = itemSettings.chatStateSendingEnabled.value_or(item.chatStateSendingEnabled);
                    item.readMarkerSendingEnabled = itemSettings.readMarkerSendingEnabled.value_or(item.readMarkerSendingEnabled);
                    item.pinningPosition = itemSettings.pinningPosition.value_or(item.pinningPosition);
                    item.automaticMediaDownloadsRule = itemSettings.automaticMediaDownloadsRule.value_or(item.automaticMediaDownloadsRule);
                });
            }
        });
}

ClientSettings AccountMigrationController::exportClientSettings()
{
    return ClientSettings(AccountController::instance()->account(), RosterModel::instance()->items());
}

QXmppTask<QXmppAccountMigrationManager::Result<>> AccountMigrationController::publishMovedStatement(const QXmppConfiguration &configuration,
                                                                                                    const QString &newAccountJid)
{
    auto client = new PublishMovedStatementClient(this);
    QXmppPromise<QXmppAccountMigrationManager::Result<>> promise;
    auto task = promise.task();

    connect(client, &PublishMovedStatementClient::finished, this, [promise](const auto &result) mutable {
        promise.finish(result);
    });

    client->publishStatement(configuration, newAccountJid);

    return task;
}

QXmppTask<QXmppAccountMigrationManager::Result<>> AccountMigrationController::notifyContacts(const QList<QString> &contactJids, const QString &oldAccountJid)
{
    if (contactJids.isEmpty()) {
        return makeReadyTask<QXmppAccountMigrationManager::Result<>>({});
    }

    QXmppPromise<QXmppAccountMigrationManager::Result<>> promise;
    auto counter = std::make_shared<int>(contactJids.size());

    for (const QString &contactJid : contactJids) {
        callRemoteTask(
            m_movedManager,
            [this, contactJid, oldAccountJid]() {
                return std::pair{m_movedManager->notifyContact(contactJid, oldAccountJid, false), this};
            },
            this,
            [promise, counter](auto &&result) mutable {
                if (promise.task().isFinished()) {
                    return;
                }

                if (const auto error = std::get_if<QXmppError>(&result)) {
                    promise.finish(*error);
                    return;
                }

                if ((--(*counter)) == 0) {
                    promise.finish(QXmpp::Success());
                }
            });
    }

    return promise.task();
}

#include "AccountMigrationController.moc"

#include "moc_AccountMigrationController.cpp"
