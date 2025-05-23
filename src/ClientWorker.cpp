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
#include <QSettings>
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
#include <QXmppUploadRequestManager.h>
#include <QXmppUtils.h>
#include <QXmppVCardManager.h>
#include <QXmppVersionManager.h>
// Kaidan
#include "AccountController.h"
#include "AtmController.h"
#include "DiscoveryManager.h"
#include "EncryptionController.h"
#include "Kaidan.h"
#include "KaidanCoreLog.h"
#include "LogHandler.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "OmemoDb.h"
#include "PresenceCache.h"
#include "RegistrationController.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
#include "TrustDb.h"

ClientWorker::ClientWorker(Database *database, bool enableLogging, QObject *parent)
    : QObject(parent)
    , m_client(new QXmppClient(QXmppClient::NoExtensions, this))
    , m_logger(new LogHandler(m_client, enableLogging, this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_omemoDb(new OmemoDb(database, this, {}, this))
{
    m_accountMigrationManager = m_client->addNewExtension<QXmppAccountMigrationManager>();
    m_client->addNewExtension<QXmppBlockingManager>();
    m_client->addNewExtension<QXmppCarbonManagerV2>();
    auto *discoveryManager = m_client->addNewExtension<QXmppDiscoveryManager>();
    m_client->addNewExtension<QXmppEntityTimeManager>();
    auto *uploadManager = m_client->addNewExtension<QXmppHttpUploadManager>(m_networkManager);
    m_client->addNewExtension<QXmppMamManager>();
    m_client->addNewExtension<QXmppPubSubManager>();
    m_movedManager = m_client->addNewExtension<QXmppMovedManager>();
    m_atmManager = m_client->addNewExtension<QXmppAtmManager>(TrustDb::instance());
    m_client->setEncryptionExtension(m_client->addNewExtension<QXmppOmemoManager>(m_omemoDb));
    m_client->addNewExtension<QXmppMessageReceiptManager>();
    m_registrationManager = m_client->addNewExtension<QXmppRegistrationManager>();
    m_rosterManager = m_client->addNewExtension<QXmppRosterManager>(m_client);
    m_client->addNewExtension<QXmppUploadRequestManager>();
    m_vCardManager = m_client->addNewExtension<QXmppVCardManager>();
    m_versionManager = m_client->addNewExtension<QXmppVersionManager>();
    m_client->addNewExtension<QXmppMixManager>();

    new DiscoveryManager(m_client, discoveryManager, this);

    // file sharing manager
    m_fileSharingManager = m_client->addNewExtension<QXmppFileSharingManager>();
    m_fileSharingManager->setMetadataGenerator(MediaUtils::generateMetadata);
    m_httpProvider = std::make_shared<QXmppHttpFileSharingProvider>(uploadManager, m_networkManager);
    m_encryptedProvider = std::make_shared<QXmppEncryptedFileSharingProvider>(m_fileSharingManager, m_httpProvider);
    m_fileSharingManager->registerProvider(m_httpProvider);
    m_fileSharingManager->registerProvider(m_encryptedProvider);

    connect(m_client, &QXmppClient::connected, this, &ClientWorker::onConnected);
    connect(m_client, &QXmppClient::disconnected, this, &ClientWorker::onDisconnected);

    connect(m_client, &QXmppClient::stateChanged, this, &ClientWorker::onConnectionStateChanged);
    connect(m_client, &QXmppClient::errorOccurred, this, &ClientWorker::onConnectionError);

    connect(m_client, &QXmppClient::credentialsChanged, this, [this]() {
        AccountController::instance()->setAuthCredentials(m_client->configuration().credentials());
    });

    connect(Kaidan::instance(), &Kaidan::logInRequested, this, &ClientWorker::logIn);
    connect(Kaidan::instance(), &Kaidan::logOutRequested, this, &ClientWorker::logOut);

    // presence
    auto *presenceCache = PresenceCache::instance();
    connect(m_client, &QXmppClient::presenceReceived, presenceCache, &PresenceCache::updatePresence);
    connect(m_client, &QXmppClient::disconnected, presenceCache, &PresenceCache::clear);

    // Reduce the network traffic when the application window is not active.
    connect(qGuiApp, &QGuiApplication::applicationStateChanged, this, [this](Qt::ApplicationState state) {
        if (state == Qt::ApplicationActive) {
            m_client->setActive(true);
        } else {
            m_client->setActive(false);
        }
    });
}

void ClientWorker::startTask(const std::function<void()> &task)
{
    if (m_client->isAuthenticated()) {
        task();
    } else {
        m_pendingTasks << task;

        // The client should only log in during regular usage and not while initializing
        // the account (e.g. not when the user's display name is changed during
        // registration). The check is needed when this method is called before a login
        // with new credentials to avoid logging in to the server before all needed
        // credentials are available. That way, the variable can already be set locally
        // during account initialization (like during registration) and set on the server
        // after the first login.
        if (!AccountController::instance()->hasNewAccount() && m_client->state() != QXmppClient::ConnectingState)
            logIn();
    }
}

void ClientWorker::finishTask()
{
    // If m_activeTasks > 0, there are still running tasks.
    // If m_activeTasks = 0, all tasks are finished (the tasks may have finished directly).
    if (m_activeTasks > 0 && --m_activeTasks == 0 && !AccountController::instance()->hasNewAccount())
        logOut();
}

void ClientWorker::logIn()
{
    auto *accountController = AccountController::instance();
    const auto account = accountController->account();
    const auto jid = account.jid;

    auto *atmController = Kaidan::instance()->atmController();
    runOnThread(atmController, [atmController, jid]() {
        atmController->setAccountJid(jid);
    });

    m_omemoDb->setAccountJid(jid);

    auto proceedLogIn = [this, accountController, account]() {
        runOnThread(EncryptionController::instance(), [this, accountController, account]() {
            EncryptionController::instance()->load().then([this, accountController, account]() {
                if (!m_isFirstLoginAfterStart || account.online) {
                    // Store the latest online state which is restored when opening Kaidan again after closing.
                    accountController->setAuthOnline(true);

                    QXmppConfiguration config;
                    config.setResource(account.jidResource());
                    config.setCredentials(account.credentials);
                    config.setPassword(account.password);
                    config.setAutoAcceptSubscriptions(false);

                    // Servers using LDAP do not support SASL PLAIN authentication.
                    config.setDisabledSaslMechanisms({});

                    connectToServer(config);
                }

                m_isFirstLoginAfterStart = false;
            });
        });
    };

    // Reset the locally cached OMEMO data after an account migration to allow a new OMEMO setup.
    if (const auto oldAccountJid = m_client->configuration().jidBare(); !oldAccountJid.isEmpty() && oldAccountJid != jid) {
        runOnThread(EncryptionController::instance(), [this, proceedLogIn]() {
            EncryptionController::instance()->resetLocally().then(this, proceedLogIn);
        });
    } else {
        proceedLogIn();
    }
}

void ClientWorker::connectToServer(QXmppConfiguration config)
{
    const auto accountController = AccountController::instance();
    const auto account = accountController->account();

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
        config.setJid(account.jid);
        config.setStreamSecurityMode(account.tlsRequirement);
        config.setIgnoreSslErrors(account.tlsErrorsIgnored);
        config.setResourcePrefix(account.resourcePrefix);
        config.setSasl2UserAgent(accountController->userAgent());

        if (!account.host.isEmpty()) {
            config.setHost(account.host);
        }

        if (account.port != PORT_AUTODETECT) {
            config.setPort(account.port);

            // Set the JID's domain part as the host if no custom host is set.
            if (account.host.isEmpty()) {
                config.setHost(QXmppUtils::jidToDomain(account.jid));
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
        Kaidan::instance()->serverFeaturesCache()->setInBandRegistrationSupported(false);

        m_client->connectToServer(config);
    }
}

void ClientWorker::logOut(bool isApplicationBeingClosed)
{
    // Store the latest online state which is restored when opening Kaidan again after closing.
    if (!isApplicationBeingClosed) {
        AccountController::instance()->setAuthOnline(false);
    }

    switch (m_client->state()) {
    case QXmppClient::DisconnectedState:
        break;
    case QXmppClient::ConnectingState:
        // Abort an ongoing registration if the application is being closed.
        // Otherwise, wait for the client to connect in order to disconnect appropriately.
        if (isApplicationBeingClosed) {
            Kaidan::instance()->registrationController()->registerOnConnectEnabled().then(this, [this](bool registerOnConnectEnabled) {
                if (registerOnConnectEnabled) {
                    m_client->disconnectFromServer();
                }
            });
        } else {
            qCDebug(KAIDAN_CORE_LOG) << "Tried to disconnect even if still connecting! Waiting for connecting to succeed and disconnect afterwards.";
            m_isDisconnecting = true;
        }

        break;
    case QXmppClient::ConnectedState:
        runOnThread(EncryptionController::instance(), [this]() {
            EncryptionController::instance()->unload().then(this, [this]() {
                m_client->disconnectFromServer();
            });
        });
    }
}

void ClientWorker::onConnected()
{
    // no mutex needed, because this is called from updateClient()
    qCDebug(KAIDAN_CORE_LOG) << "Connected to server";

    // If there was an error before, notify about its absence.
    Q_EMIT connectionErrorChanged(ClientWorker::NoError);

    runOnThread(
        AccountController::instance(),
        []() {
            return AccountController::instance()->handleConnected();
        },
        this,
        [this](bool handled) {
            if (handled) {
                return;
            }

            // Try to complete pending tasks which could not be completed while the client was
            // offline and skip further normal tasks if at least one was completed after a login
            // with old credentials.
            if (startPendingTasks())
                return;

            // If the client was connecting during a disconnection attempt, disconnect it now.
            if (m_isDisconnecting) {
                m_isDisconnecting = false;
                logOut();
                return;
            }

            // The following tasks are only done after a login with new credentials or connection
            // settings.
            if (AccountController::instance()->hasNewAccount()) {
                Q_EMIT loggedInWithNewCredentials();

                // Store the valid connection data.
                AccountController::instance()->storeAccount();
            }

            // Enable auto reconnection so that the client is always trying to reconnect
            // automatically in case of a connection outage.
            m_client->configuration().setAutoReconnectionEnabled(true);

            const auto jid = AccountController::instance()->account().jid;

            // Send message that could not be sent yet because the client was offline.
            runOnThread(MessageController::instance(), [jid]() {
                MessageController::instance()->sendPendingMessages();
            });

            // Send read markers that could not be sent yet because the client was offline.
            runOnThread(RosterModel::instance(), [jid]() {
                RosterModel::instance()->sendPendingReadMarkers(jid);
            });

            // Send message reactions that could not be sent yet because the client was offline.
            runOnThread(MessageController::instance(), [jid]() {
                MessageController::instance()->sendPendingMessageReactions(jid);
            });

            runOnThread(EncryptionController::instance(), []() {
                EncryptionController::instance()->setUp();
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

    runOnThread(AccountController::instance(), []() {
        AccountController::instance()->handleDisconnected();
    });
}

void ClientWorker::onConnectionStateChanged(QXmppClient::State connectionState)
{
    Q_EMIT connectionStateChanged(Enums::ConnectionState(connectionState));
}

void ClientWorker::onConnectionError(const QXmppError &error)
{
    // no mutex needed, because this is called from updateClient()
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
            Q_EMIT connectionErrorChanged(ClientWorker::AuthenticationFailed);
            return;
        }
    }

    Q_EMIT connectionErrorChanged(ClientWorker::UnknownError);
}

bool ClientWorker::startPendingTasks()
{
    bool isBusy = false;

    while (!m_pendingTasks.isEmpty()) {
        m_pendingTasks.takeFirst()();
        m_activeTasks++;

        isBusy = true;
    }

    return !AccountController::instance()->hasNewAccount() && isBusy;
}

#include "moc_ClientWorker.cpp"
