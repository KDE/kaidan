// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ClientWorker.h"
// Qt
#include <QGuiApplication>
#include <QSettings>
#include <QNetworkAccessManager>
// QXmpp
#include <QXmppBlockingManager.h>
#include <QXmppCarbonManagerV2.h>
#include <QXmppEncryptedFileSharingProvider.h>
#include <QXmppFileSharingManager.h>
#include <QXmppHttpFileSharingProvider.h>
#include <QXmppHttpUploadManager.h>
#include <QXmppMamManager.h>
#include <QXmppPubSubBaseItem.h>
#include <QXmppPubSubManager.h>
#include <QXmppUploadRequestManager.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "AtmManager.h"
#include "AvatarFileStorage.h"
#include "ChatHintModel.h"
#include "DiscoveryManager.h"
#include "Enums.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "LogHandler.h"
#include "MessageHandler.h"
#include "MessageModel.h"
#include "OmemoCache.h"
#include "OmemoManager.h"
#include "PresenceCache.h"
#include "RegistrationManager.h"
#include "RosterManager.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
#include "VCardCache.h"
#include "VCardManager.h"
#include "VersionManager.h"
#include "Settings.h"
#include "MediaUtils.h"

ClientWorker::Caches::Caches(QObject *parent)
	: settings(new Settings(parent)),
	  vCardCache(new VCardCache(parent)),
	  accountManager(new AccountManager(settings, vCardCache, parent)),
	  presenceCache(new PresenceCache(parent)),
	  msgModel(new MessageModel(parent)),
	  rosterModel(new RosterModel(parent)),
	  chatHintModel(new ChatHintModel(parent)),
	  omemoCache(new OmemoCache(parent)),
	  avatarStorage(new AvatarFileStorage(parent)),
	  serverFeaturesCache(new ServerFeaturesCache(parent))
{
}

ClientWorker::ClientWorker(Caches *caches, Database *database, bool enableLogging, QObject* parent)
	: QObject(parent),
	  m_caches(caches),
	  m_client(new QXmppClient(this)),
	  m_logger(new LogHandler(m_client, enableLogging, this)),
	  m_enableLogging(enableLogging),
	  m_networkManager(new QNetworkAccessManager(this))
{
	m_client->addNewExtension<QXmppCarbonManagerV2>();
	m_client->addNewExtension<QXmppMamManager>();
	m_client->addNewExtension<QXmppPubSubManager>();
	m_client->addNewExtension<QXmppUploadRequestManager>();
	m_client->addNewExtension<QXmppBlockingManager>();
	auto *uploadManager = m_client->addNewExtension<QXmppHttpUploadManager>(m_networkManager);

	m_registrationManager = new RegistrationManager(this, m_client, this);
	m_vCardManager = new VCardManager(this, m_client, m_caches->avatarStorage, this);
	m_rosterManager = new RosterManager(this, m_client, this);
	m_messageHandler = new MessageHandler(this, m_client, this);
	m_atmManager = new AtmManager(m_client, database, this);
	m_omemoManager = new OmemoManager(m_client, database, this);
	m_discoveryManager = new DiscoveryManager(m_client, this);
	m_versionManager = new VersionManager(m_client, this);

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
	connect(m_client, &QXmppClient::error, this, &ClientWorker::onConnectionError);

	connect(Kaidan::instance(), &Kaidan::logInRequested, this, &ClientWorker::logIn);
	connect(Kaidan::instance(), &Kaidan::registrationFormRequested, this, &ClientWorker::connectToRegister);
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

	// account deletion
	connect(Kaidan::instance(), &Kaidan::deleteAccountFromClient, this, &ClientWorker::deleteAccountFromClient);
	connect(Kaidan::instance(), &Kaidan::deleteAccountFromClientAndServer, this, &ClientWorker::deleteAccountFromClientAndServer);
}

void ClientWorker::startTask(const std::function<void ()> &task)
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
		if (!AccountManager::instance()->hasNewCredentials() && m_client->state() != QXmppClient::ConnectingState)
			logIn();
	}
}

void ClientWorker::finishTask()
{
	// If m_activeTasks > 0, there are still running tasks.
	// If m_activeTasks = 0, all tasks are finished (the tasks may have finished directly).
	if (m_activeTasks > 0 && --m_activeTasks == 0 && !AccountManager::instance()->hasNewCredentials())
		logOut();
}

void ClientWorker::logIn()
{
	const auto jid = AccountManager::instance()->jid();
	m_atmManager->setAccountJid(jid);
	m_omemoManager->setAccountJid(jid);

	await(m_omemoManager->load(), this, [this]() {
		if (!m_isFirstLoginAfterStart || m_caches->settings->authOnline()) {
			// Store the latest online state which is restored when opening Kaidan again after closing.
			m_caches->settings->setAuthOnline(true);

			QXmppConfiguration config;
			config.setResource(AccountManager::instance()->jidResource());
			config.setPassword(AccountManager::instance()->password());
			config.setAutoAcceptSubscriptions(false);
			config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);

			connectToServer(config);
		}

		m_isFirstLoginAfterStart = false;
	});
}

void ClientWorker::connectToRegister()
{
	AccountManager::instance()->setHasNewCredentials(true);
	m_registrationManager->setRegisterOnConnectEnabled(true);
	connectToServer();
}

void ClientWorker::connectToServer(QXmppConfiguration config)
{
	switch (m_client->state()) {
	case QXmppClient::ConnectingState:
		qDebug() << "[main] Tried to connect even if already connecting! Nothing is done.";
		break;
	case QXmppClient::ConnectedState:
		qDebug() << "[main] Tried to connect even if already connected! Disconnecting first and connecting afterwards.";
		m_isReconnecting = true;
		m_configToBeUsedOnNextConnect = config;
		logOut();
		break;
	case QXmppClient::DisconnectedState:
		config.setJid(AccountManager::instance()->jid());
		config.setStreamSecurityMode(QXmppConfiguration::TLSRequired);

		auto host = AccountManager::instance()->host();
		if (!host.isEmpty())
			config.setHost(host);

		auto port = AccountManager::instance()->port();
		if (port != PORT_AUTODETECT) {
			config.setPort(port);

			// Set the JID's domain part as the host if no custom host is set.
			if (host.isEmpty())
				config.setHost(QXmppUtils::jidToDomain(AccountManager::instance()->jid()));
		}

		// Disable the automatic reconnection in case this connection attempt is not
		// successful. Otherwise, this could result in a reconnection loop. It is enabled
		// again after a successful login.
		config.setAutoReconnectionEnabled(false);

		// Reset the attribute for In-Band Registration support. That is needed when the
		// attribute was true until the last logout but the server disabled the support
		// afterwards. Without that reset, the attribute would stay "true" even if the
		// server did not support it anymore.
		m_caches->serverFeaturesCache->setInBandRegistrationSupported(false);

		m_client->connectToServer(config);
	}
}

void ClientWorker::logOut(bool isApplicationBeingClosed)
{
	// Store the latest online state which is restored when opening Kaidan again after closing.
	if (!isApplicationBeingClosed)
		m_caches->settings->setAuthOnline(false);

	switch (m_client->state()) {
	case QXmppClient::DisconnectedState:
		break;
	case QXmppClient::ConnectingState:
		qDebug() << "[main] Tried to disconnect even if still connecting! Waiting for connecting to succeed and disconnect afterwards.";
		m_isDisconnecting = true;
		break;
	case QXmppClient::ConnectedState:
		if (AccountManager::instance()->hasNewCredentials()) {
			AccountManager::instance()->setHasNewCredentials(false);
		}

		if (AccountManager::instance()->hasNewConnectionSettings()) {
			AccountManager::instance()->setHasNewConnectionSettings(false);
		}

		await(m_omemoManager->unsubscribeFromDeviceLists(), this, [this] {
			m_client->disconnectFromServer();
		});
	}
}

void ClientWorker::deleteAccountFromClientAndServer()
{
	m_isAccountToBeDeletedFromClientAndServer = true;

	// If the client is already connected, delete the account directly from the server.
	// Otherwise, connect first and delete the account afterwards.
	if (m_client->isAuthenticated()) {
		m_registrationManager->deleteAccount();
	} else {
		m_isClientConnectedBeforeAccountDeletionFromServer = false;
		logIn();
	}
}

void ClientWorker::deleteAccountFromClient()
{
	// If the client is already disconnected, delete the account directly from the client.
	// Otherwise, disconnect first and delete the account afterwards.
	if (!m_client->isAuthenticated()) {
		AccountManager::instance()->removeAccount(m_client->configuration().jidBare());
		m_isAccountToBeDeletedFromClient = false;
	} else {
		m_omemoManager->resetOwnDevice().then(this, [this](auto &&) {
			m_isAccountToBeDeletedFromClient = true;
			logOut();
		});
	}
}

void ClientWorker::handleAccountDeletedFromServer()
{
	m_isAccountDeletedFromServer = true;
}

void ClientWorker::handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error)
{
	Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Your account could not be deleted from the server. Therefore, it was also not removed from this app: %1").arg(error.text()));

	m_isAccountToBeDeletedFromClientAndServer = false;

	if (!m_isClientConnectedBeforeAccountDeletionFromServer)
		logOut();
}

void ClientWorker::onConnected()
{
	// no mutex needed, because this is called from updateClient()
	qDebug() << "[client] Connected successfully to server";

	// If there was an error before, notify about its absence.
	Q_EMIT connectionErrorChanged(ClientWorker::NoError);

	// If the account could not be deleted from the server because the client was
	// disconnected, delete it now.
	if (m_isAccountToBeDeletedFromClientAndServer) {
		m_registrationManager->deleteAccount();
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

	// The following tasks are only done after a login with new credentials or connection settings.
	if (AccountManager::instance()->hasNewCredentials() || AccountManager::instance()->hasNewConnectionSettings()) {
		if (AccountManager::instance()->hasNewCredentials()) {
			Q_EMIT loggedInWithNewCredentials();
		}

		// Store the valid settings.
		AccountManager::instance()->storeConnectionData();
	}

	// Enable auto reconnection so that the client is always trying to reconnect
	// automatically in case of a connection outage.
	m_client->configuration().setAutoReconnectionEnabled(true);

	m_messageHandler->sendPendingMessages();

	// Send read markers that could not be sent yet because the client was offline.
	runOnThread(RosterModel::instance(), [jid = AccountManager::instance()->jid()]() {
		RosterModel::instance()->sendPendingReadMarkers(jid);
	});

	// Send message reactions that could not be sent yet because the client was offline.
	runOnThread(RosterModel::instance(), [jid = AccountManager::instance()->jid()]() {
		MessageModel::instance()->sendPendingMessageReactions(jid);
	});

	m_omemoManager->setUp();
}

void ClientWorker::onDisconnected()
{
	if (m_isReconnecting) {
		m_isReconnecting = false;
		connectToServer(m_configToBeUsedOnNextConnect);
		m_configToBeUsedOnNextConnect = {};
		return;
	}

	// Delete the account from the client if the client was connected and had to
	// disconnect first or if the account was deleted from the server.
	if (m_isAccountToBeDeletedFromClient || (m_isAccountToBeDeletedFromClientAndServer && m_isAccountDeletedFromServer)) {
		m_isAccountToBeDeletedFromClientAndServer = false;
		m_isAccountDeletedFromServer = false;

		deleteAccountFromClient();
	}
}

void ClientWorker::onConnectionStateChanged(QXmppClient::State connectionState)
{
	Q_EMIT connectionStateChanged(Enums::ConnectionState(connectionState));
}

void ClientWorker::onConnectionError(QXmppClient::Error error)
{
	// no mutex needed, because this is called from updateClient()
	qDebug() << "[client] Disconnected:" << error;

	switch (error) {
	case QXmppClient::NoError:
		Q_UNREACHABLE();
	case QXmppClient::KeepAliveError:
		Q_EMIT connectionErrorChanged(ClientWorker::KeepAliveError);
		break;
	case QXmppClient::XmppStreamError: {
		QXmppStanza::Error::Condition xmppStreamError = m_client->xmppStreamError();
		qDebug() << "[client] XMPP stream error:" << xmppStreamError;
		if (xmppStreamError == QXmppStanza::Error::NotAuthorized) {
			Q_EMIT connectionErrorChanged(ClientWorker::AuthenticationFailed);
		} else {
			Q_EMIT connectionErrorChanged(ClientWorker::NotConnected);
		}
		break;
	}
	case QXmppClient::SocketError: {
		QAbstractSocket::SocketError socketError = m_client->socketError();
		switch (socketError) {
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
		break;
	}
	}
}

bool ClientWorker::startPendingTasks()
{
	bool isBusy = false;

	while (!m_pendingTasks.isEmpty()) {
		m_pendingTasks.takeFirst()();
		m_activeTasks++;

		isBusy = true;
	}

	return !AccountManager::instance()->hasNewCredentials() && isBusy;
}
