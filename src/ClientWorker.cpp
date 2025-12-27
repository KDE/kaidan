// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ClientWorker.h"

// Qt
#include <QGuiApplication>
#include <QNetworkAccessManager>
// QXmpp
#include <QXmppAccountMigrationManager.h>
#include <QXmppAtmManager.h>
#include <QXmppAuthenticationError.h>
#include <QXmppBlockingManager.h>
#include <QXmppCarbonManagerV2.h>
#include <QXmppCredentials.h>
#include <QXmppDiscoveryManager.h>
#include <QXmppEncryptedFileSharingProvider.h>
#include <QXmppEntityTimeManager.h>
#include <QXmppFileSharingManager.h>
#include <QXmppHttpFileSharingProvider.h>
#include <QXmppHttpUploadManager.h>
#include <QXmppMamManager.h>
#include <QXmppMixManager.h>
#include <QXmppMovedManager.h>
#include <QXmppOmemoManager.h>
#include <QXmppPubSubBaseItem.h>
#include <QXmppPubSubManager.h>
#include <QXmppRegistrationManager.h>
#include <QXmppRosterManager.h>
#include <QXmppSasl2UserAgent.h>
#include <QXmppStreamError.h>
#include <QXmppUtils.h>
#include <QXmppVCardManager.h>
#include <QXmppVersionManager.h>
// Kaidan
#include "Account.h"
#include "AtmController.h"
#include "DiscoveryController.h"
#include "EncryptionController.h"
#include "KaidanCoreLog.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "OmemoDb.h"
#include "PresenceCache.h"
#include "RegistrationController.h"
#include "TrustDb.h"

ClientWorker::ClientWorker(AccountSettings *accountSettings, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_client(new QXmppClient(QXmppClient::NoExtensions, this))
{
    auto *networkAccessManager = new QNetworkAccessManager(this);

    m_accountMigrationManager = m_client->addNewExtension<QXmppAccountMigrationManager>();
    m_blockingManager = m_client->addNewExtension<QXmppBlockingManager>();
    m_client->addNewExtension<QXmppCarbonManagerV2>();
    m_discoveryManager = m_client->addNewExtension<QXmppDiscoveryManager>();
    m_client->addNewExtension<QXmppEntityTimeManager>();
    m_uploadManager = m_client->addNewExtension<QXmppHttpUploadManager>(networkAccessManager);
    m_mamManager = m_client->addNewExtension<QXmppMamManager>();
    m_client->addNewExtension<QXmppPubSubManager>();
    m_movedManager = m_client->addNewExtension<QXmppMovedManager>();
    m_atmManager = m_client->addNewExtension<QXmppAtmManager>(new TrustDb(m_accountSettings, this, this));
    m_omemoManager = m_client->addNewExtension<QXmppOmemoManager>(new OmemoDb(m_accountSettings, this, this));
    m_client->setEncryptionExtension(m_omemoManager);
    m_messageReceiptManager = m_client->addNewExtension<QXmppMessageReceiptManager>();
    m_registrationManager = m_client->addNewExtension<QXmppRegistrationManager>();
    m_rosterManager = m_client->addNewExtension<QXmppRosterManager>(m_client);
    m_vCardManager = m_client->addNewExtension<QXmppVCardManager>();
    m_versionManager = m_client->addNewExtension<QXmppVersionManager>();
    m_mixManager = m_client->addNewExtension<QXmppMixManager>();

    // file sharing manager
    m_fileSharingManager = m_client->addNewExtension<QXmppFileSharingManager>();
    m_fileSharingManager->setMetadataGenerator(MediaUtils::generateMetadata);
    m_httpProvider = std::make_shared<QXmppHttpFileSharingProvider>(m_uploadManager, networkAccessManager);
    m_encryptedProvider = std::make_shared<QXmppEncryptedFileSharingProvider>(m_fileSharingManager, m_httpProvider);
    m_fileSharingManager->registerProvider(m_httpProvider);
    m_fileSharingManager->registerProvider(m_encryptedProvider);

    connect(m_client, &QXmppClient::connected, this, &ClientWorker::onConnected);
    connect(m_client, &QXmppClient::disconnected, this, &ClientWorker::onDisconnected);

    connect(m_client, &QXmppClient::stateChanged, this, &ClientWorker::onConnectionStateChanged);
    connect(m_client, &QXmppClient::errorOccurred, this, &ClientWorker::onConnectionError);

    connect(m_client, &QXmppClient::credentialsChanged, this, [this]() {
        runOnThread(m_accountSettings, [this, credentials = m_client->configuration().credentials()]() {
            m_accountSettings->setCredentials(credentials);
        });
    });

    // Reduce the network traffic when the application window is not active.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            m_client->setActive(true);
        } else {
            m_client->setActive(false);
        }
    });
}

void ClientWorker::initialize(AtmController *atmController,
                              EncryptionController *encryptionController,
                              MessageController *messageController,
                              RegistrationController *registrationController,
                              PresenceCache *presenceCache)
{
    m_atmController = atmController;
    m_encryptionController = encryptionController;
    m_messageController = messageController;
    m_registrationController = registrationController;
    m_presenceCache = presenceCache;

    connect(m_client, &QXmppClient::presenceReceived, m_presenceCache, &PresenceCache::updatePresence);
    connect(m_client, &QXmppClient::disconnected, m_presenceCache, &PresenceCache::clear);
}

void ClientWorker::logIn()
{
    runOnThread(m_accountSettings, [this]() {
        if (m_accountSettings->password().isEmpty() && m_accountSettings->credentials() == QXmppCredentials()) {
            Q_EMIT connectionErrorChanged(ClientWorker::AuthenticationFailed);
            return;
        }

        m_encryptionController->load()
            .then(m_accountSettings,
                  [this] {
                      QXmppConfiguration config;
                      config.setResource(m_accountSettings->jidResource());
                      config.setCredentials(m_accountSettings->credentials());
                      config.setPassword(m_accountSettings->password());
                      config.setSasl2UserAgent(m_accountSettings->userAgent());
                      config.setAutoAcceptSubscriptions(false);

                      // Servers using LDAP need SASL PLAIN authentication.
                      if (m_accountSettings->plainAuthAllowed()) {
                          config.setDisabledSaslMechanisms({});
                      }

                      return config;
                  })
            .then(this, [this](QXmppConfiguration &&config) {
                connectToServer(std::move(config));
            });
    });
}

void ClientWorker::connectToServer(QXmppConfiguration config)
{
    switch (m_client->state()) {
    case QXmppClient::ConnectingState:
        qCDebug(KAIDAN_CORE_LOG) << "Tried to connect even if already connecting! Nothing is done.";
        break;
    case QXmppClient::ConnectedState:
        qCDebug(KAIDAN_CORE_LOG) << "Tried to connect even if already connected! Disconnecting first and connecting afterwards.";
        m_isReconnecting = true;
        m_configToBeUsedOnNextConnect = config;
        logOut();
        break;
    case QXmppClient::DisconnectedState:
        runOnThread(
            m_accountSettings,
            [this, config]() mutable {
                config.setJid(m_accountSettings->jid());
                config.setStreamSecurityMode(m_accountSettings->tlsRequirement());
                config.setIgnoreSslErrors(m_accountSettings->tlsErrorsIgnored());

                if (!m_accountSettings->host().isEmpty()) {
                    config.setHost(m_accountSettings->host());
                }

                if (m_accountSettings->port() != AUTO_DETECT_PORT) {
                    config.setPort(m_accountSettings->port());

                    // Set the JID's domain part as the host if no custom host is set.
                    if (m_accountSettings->host().isEmpty()) {
                        config.setHost(QXmppUtils::jidToDomain(m_accountSettings->jid()));
                    }
                }

                // Disable the automatic reconnection in case this connection attempt is not
                // successful. Otherwise, this could result in a reconnection loop. It is enabled
                // again after a successful login.
                config.setAutoReconnectionEnabled(false);

                // Reset the attribute for In-Band Registration support. That is needed when the
                // attribute was true until the last logout but the server disabled the support
                // afterwards. Without that reset, the attribute would stay "true" even if the
                // server did not support it anymore.
                m_accountSettings->setInBandRegistrationFeaturesSupported(false);

                return config;
            },
            this,
            [this](QXmppConfiguration &&config) {
                m_client->connectToServer(config);
            });
    }
}

void ClientWorker::logOut(bool isApplicationBeingClosed)
{
    switch (m_client->state()) {
    case QXmppClient::DisconnectedState:
        break;
    case QXmppClient::ConnectingState:
        // Abort an ongoing registration if the application is being closed.
        // Otherwise, wait for the client to connect in order to disconnect appropriately.
        if (isApplicationBeingClosed) {
            runOnThread(m_registrationController, [this]() {
                m_registrationController->registerOnConnectEnabled().then(this, [this](bool registerOnConnectEnabled) {
                    if (registerOnConnectEnabled) {
                        m_client->disconnectFromServer();
                    }
                });
            });
        } else {
            qCDebug(KAIDAN_CORE_LOG) << "Tried to disconnect even if still connecting! Waiting for connecting to succeed and disconnect afterwards.";
            m_isDisconnecting = true;
        }

        break;
    case QXmppClient::ConnectedState:
        runOnThread(m_encryptionController, [this]() {
            m_encryptionController->unload().then(this, [this]() {
                m_client->disconnectFromServer();
            });
        });
    }
}

void ClientWorker::onConnected()
{
    qCDebug(KAIDAN_CORE_LOG) << "Connected to server";

    // If there was an error before, notify about its absence.
    Q_EMIT connectionErrorChanged(ClientWorker::NoError);

    runOnThread(
        m_registrationController,
        [this]() {
            return m_registrationController->handleConnected();
        },
        this,
        [this](bool handled) {
            if (handled) {
                return;
            }

            // If the client was connecting during a disconnection attempt, disconnect it now.
            if (m_isDisconnecting) {
                m_isDisconnecting = false;
                logOut();
                return;
            }

            runOnThread(m_accountSettings, [this]() {
                // Store the valid connection data and other temporary data such as the name.
                m_accountSettings->storeTemporaryData();
            });

            // Enable auto reconnection so that the client is always trying to reconnect
            // automatically in case of a connection outage.
            m_client->configuration().setAutoReconnectionEnabled(true);

            runOnThread(m_encryptionController, [this]() {
                m_encryptionController->setUp().then(m_messageController, [this]() {
                    if (m_uploadManager->support() == QXmppHttpUploadManager::Support::Unknown) {
                        connect(m_uploadManager,
                                &QXmppHttpUploadManager::servicesChanged,
                                m_messageController,
                                &MessageController::sendPendingData,
                                Qt::SingleShotConnection);
                    } else {
                        m_messageController->sendPendingData();
                    }
                });
            });
        });
}

void ClientWorker::onDisconnected()
{
    qCDebug(KAIDAN_CORE_LOG) << "Disconnected from server";

    if (m_isReconnecting) {
        m_isReconnecting = false;
        connectToServer(m_configToBeUsedOnNextConnect);
        m_configToBeUsedOnNextConnect = {};
        return;
    }

    runOnThread(m_registrationController, [this]() {
        m_registrationController->handleDisconnected();
    });
}

void ClientWorker::onConnectionStateChanged(QXmppClient::State connectionState)
{
    Q_EMIT connectionStateChanged(Enums::ConnectionState(connectionState));
}

void ClientWorker::onConnectionError(const QXmppError &error)
{
    qCDebug(KAIDAN_CORE_LOG) << "Connection error:" << error.description;

    if (const auto socketError = error.value<QAbstractSocket::SocketError>()) {
        switch (*socketError) {
        case QAbstractSocket::ConnectionRefusedError:
        case QAbstractSocket::RemoteHostClosedError:
            Q_EMIT connectionErrorChanged(ClientWorker::ConnectionRefused);
            break;
        case QAbstractSocket::HostNotFoundError:
            Q_EMIT connectionErrorChanged(ClientWorker::DnsError);
            break;
        case QAbstractSocket::SocketAccessError:
            Q_EMIT connectionErrorChanged(ClientWorker::NoNetworkPermission);
            break;
        case QAbstractSocket::SocketTimeoutError:
            Q_EMIT connectionErrorChanged(ClientWorker::KeepAliveError);
            break;
        case QAbstractSocket::SslHandshakeFailedError:
        case QAbstractSocket::SslInternalError:
            Q_EMIT connectionErrorChanged(ClientWorker::TlsFailed);
            break;
        default:
            Q_EMIT connectionErrorChanged(ClientWorker::NotConnected);
        }
        return;
    } else if (error.holdsType<QXmpp::TimeoutError>()) {
        Q_EMIT connectionErrorChanged(ClientWorker::KeepAliveError);
        return;
    } else if (error.holdsType<QXmpp::StreamError>()) {
        Q_EMIT connectionErrorChanged(ClientWorker::NotConnected);
        return;
    } else if (const auto authenticationError = error.value<QXmpp::AuthenticationError>()) {
        const auto type = authenticationError->type;
        const auto text = authenticationError->text;

        // Some servers require new users to confirm their account creation via instructions within
        // an email message sent by the server to the user after a successful registration.
        // That usually involves opening a URL.
        // Afterwards, the account is activated and the user can log in.
        if ((type == QXmpp::AuthenticationError::AccountDisabled || type == QXmpp::AuthenticationError::NotAuthorized)
            && text.contains(QStringLiteral("activat")) && text.contains(QStringLiteral("mail"))) {
            Q_EMIT connectionErrorChanged(ClientWorker::EmailConfirmationRequired);
            return;
        } else if (type == QXmpp::AuthenticationError::NotAuthorized) {
            runOnThread(m_accountSettings, [this]() {
                m_accountSettings->setCredentials({});
            });

            Q_EMIT connectionErrorChanged(ClientWorker::AuthenticationFailed);
            return;
        }
    }

    Q_EMIT connectionErrorChanged(ClientWorker::UnknownError);
}

#include "moc_ClientWorker.cpp"
