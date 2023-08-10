// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <functional>
// Qt
// QXmpp
#include <QXmppClient.h>
// Kaidan
#include "Enums.h"

class AccountManager;
class AtmManager;
class AvatarFileStorage;
class Database;
class DiscoveryManager;
class LogHandler;
class MessageHandler;
class MessageModel;
class OmemoCache;
class OmemoManager;
class RegistrationManager;
class RosterManager;
class RosterModel;
class ServerFeaturesCache;
class VCardCache;
class VCardManager;
class VersionManager;
class Settings;
class QNetworkAccessManager;
class QXmppFileSharingManager;
class QXmppHttpFileSharingProvider;
class QXmppEncryptedFileSharingProvider;
class PresenceCache;

/**
 * The ClientWorker is used as a QObject-based worker on the ClientThread.
 */
class ClientWorker : public QObject
{
	Q_OBJECT
	Q_PROPERTY(RegistrationManager* registrationManager READ registrationManager CONSTANT)
	Q_PROPERTY(VCardManager* vCardManager READ vCardManager CONSTANT)
	Q_PROPERTY(RosterManager* rosterManager READ rosterManager CONSTANT)
	Q_PROPERTY(MessageHandler* messageHandler READ messageHandler CONSTANT)
	Q_PROPERTY(DiscoveryManager* discoveryManager READ discoveryManager CONSTANT)
	Q_PROPERTY(VersionManager* versionManager READ versionManager CONSTANT)
	Q_PROPERTY(AtmManager* atmManager READ atmManager CONSTANT)
	Q_PROPERTY(OmemoManager* omemoManager READ omemoManager CONSTANT)

public:
	/**
	 * enumeration of possible connection errors
	 */
	enum ConnectionError {
		NoError,
		AuthenticationFailed,
		NotConnected,
		TlsFailed,
		TlsNotAvailable,
		DnsError,
		ConnectionRefused,
		NoSupportedAuth,
		KeepAliveError,
		NoNetworkPermission,
		RegistrationUnsupported
	};
	Q_ENUM(ConnectionError)

	struct Caches {
		Caches(QObject *parent = nullptr);

		Settings *settings;
		VCardCache *vCardCache;
		AccountManager *accountManager;
		PresenceCache *presenceCache;
		MessageModel *msgModel;
		RosterModel *rosterModel;
		OmemoCache *omemoCache;
		AvatarFileStorage *avatarStorage;
		ServerFeaturesCache *serverFeaturesCache;
	};

	/**
	 * @param caches All caches running in the main thread for communication with the UI.
	 * @param enableLogging If logging of the XMPP stream should be done.
	 * @param parent Optional QObject-based parent.
	 */
	ClientWorker(Caches *caches, Database *database, bool enableLogging, QObject *parent = nullptr);

	RegistrationManager *registrationManager() const
	{
		return m_registrationManager;
	}

	VCardManager *vCardManager() const
	{
		return m_vCardManager;
	}

	RosterManager *rosterManager() const
	{
		return m_rosterManager;
	}

	MessageHandler *messageHandler() const
	{
		return m_messageHandler;
	}

	DiscoveryManager *discoveryManager() const
	{
		return m_discoveryManager;
	}

	VersionManager *versionManager() const
	{
		return m_versionManager;
	}

	AtmManager *atmManager() const
	{
		return m_atmManager;
	}

	OmemoManager *omemoManager() const
	{
		return m_omemoManager;
	}

	Caches *caches() const
	{
		return m_caches;
	}

	QXmppFileSharingManager *fileSharingManager() const
	{
		return m_fileSharingManager;
	}

	std::shared_ptr<QXmppHttpFileSharingProvider> httpFileSharingProvider() const
	{
		return m_httpProvider;
	}

	std::shared_ptr<QXmppEncryptedFileSharingProvider> encryptedHttpFileSharingProvider() const
	{
		return m_encryptedProvider;
	}

	QXmppClient *xmppClient() const
	{
		return m_client;
	}

	/**
	 * Starts or enqueues a task which will be executed after successful login (e.g. a
	 * nickname change).
	 *
	 * This method is called by managers which must call "finishTask()" as soon as the
	 * task is completed.
	 *
	 * If the user is logged out when this method is called, a login is triggered, the
	 * task is started and a logout is triggered afterwards. However, if this method is
	 * called before a login with new credentials (e.g. during account registration), the
	 * task is started after the subsequent login.
	 *
	 * @param task task which is run directly if the user is logged in or enqueued to be
	 * run after an automatic login
	 */
	void startTask(const std::function<void ()> &task);

	/**
	 * Finishes a task started by "startTask()".
	 *
	 * This must be called after a possible completion of a pending task.
	 *
	 * A logout is triggered when this method is called after the second login with the
	 * same credentials or later. That means, a logout is not triggered after a login with
	 * new credentials (e.g. after a registration).
	 */
	void finishTask();

	/**
	 * Connects to the server and logs in with all needed configuration variables.
	 */
	Q_INVOKABLE void logIn();

	/**
	 * Connects to the server and requests a data form for account registration.
	 */
	Q_INVOKABLE void connectToRegister();

	/**
	 * Connects to the server with a minimal configuration.
	 *
	 * Some additional configuration variables can be set by passing a configuration.
	 *
	 * @param config configuration with additional variables for connecting to the server
	 * or nothing if only the minimal configuration should be used
	 */
	Q_INVOKABLE void connectToServer(QXmppConfiguration config = QXmppConfiguration());

	/**
	 * Logs out of the server if the client is not already logged out.
	 *
	 * @param isApplicationBeingClosed true if the application will be terminated directly after logging out, false otherwise
	 */
	Q_INVOKABLE void logOut(bool isApplicationBeingClosed = false);

	/**
	 * Deletes the account data from the client and server.
	 */
	Q_INVOKABLE void deleteAccountFromClientAndServer();

	/**
	 * Deletes the account data from the configuration file and database.
	 */
	Q_INVOKABLE void deleteAccountFromClient();

	/**
	 * Called when the account is deleted from the server.
	 */
	Q_INVOKABLE void handleAccountDeletedFromServer();

	/**
	 * Called when the account could not be deleted from the server.
	 *
	 * @param error error of the failed account deletion
	 */
	Q_INVOKABLE void handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error);

	/**
	 * Emitted when an authenticated connection to the server is established with new
	 * credentials for the first time.
	 *
	 * The client will be in connected state when this is emitted.
	 */
	Q_SIGNAL void loggedInWithNewCredentials();

	/**
	 * Emitted when the client's connection state changed.
	 *
	 * @param connectionState new connection state
	 */
	Q_SIGNAL void connectionStateChanged(Enums::ConnectionState connectionState);

	/**
	 * Emitted when the client failed to connect to the server.
	 *
	 * @param error new connection error
	 */
	Q_SIGNAL void connectionErrorChanged(ClientWorker::ConnectionError error);

private:
	/**
	 * Called when an authenticated connection to the server is established.
	 */
	void onConnected();

	/**
	 * Called when the connection to the server is closed.
	 */
	void onDisconnected();

	/**
	 * Handles the change of the connection state.
	 *
	 * @param connectionState new connection state
	 */
	void onConnectionStateChanged(QXmppClient::State connectionState);

	/**
	 * Handles a connection error.
	 *
	 * @param error new connection error
	 */
	void onConnectionError(QXmppClient::Error error);

	/**
	 * Starts a pending (enqueued) task (e.g. a password change) if the variable (e.g. a
	 * password) could not be changed on the server before because the client was not
	 * logged in.
	 *
	 * @return true if at least one pending task is started on the second login with the
	 * same credentials or later, otherwise false
	 */
	bool startPendingTasks();

	Caches *m_caches;
	QXmppClient *m_client;
	LogHandler *m_logger;
	bool m_enableLogging;
	QNetworkAccessManager *m_networkManager;

	RegistrationManager *m_registrationManager;
	VCardManager *m_vCardManager;
	RosterManager *m_rosterManager;
	AtmManager *m_atmManager;
	OmemoManager *m_omemoManager;
	MessageHandler *m_messageHandler;
	DiscoveryManager *m_discoveryManager;
	VersionManager *m_versionManager;

	QXmppFileSharingManager *m_fileSharingManager;
	std::shared_ptr<QXmppHttpFileSharingProvider> m_httpProvider;
	std::shared_ptr<QXmppEncryptedFileSharingProvider> m_encryptedProvider;
	QList<std::function<void ()>> m_pendingTasks;
	uint m_activeTasks = 0;

	bool m_isFirstLoginAfterStart = true;
	bool m_isReconnecting = false;
	bool m_isDisconnecting = false;
	QXmppConfiguration m_configToBeUsedOnNextConnect;

	// These variables are used for checking the state of an ongoing account deletion.
	bool m_isAccountToBeDeletedFromClient = false;
	bool m_isAccountToBeDeletedFromClientAndServer = false;
	bool m_isAccountDeletedFromServer = false;
	bool m_isClientConnectedBeforeAccountDeletionFromServer = true;
};
