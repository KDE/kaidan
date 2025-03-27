// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppClient.h>
// Kaidan
#include "Enums.h"

class AvatarFileStorage;
class Database;
class DiscoveryManager;
class LogHandler;
class OmemoDb;
class PresenceCache;
class RosterModel;
class ServerFeaturesCache;
class Settings;
class VCardCache;
class QNetworkAccessManager;
class QXmppAccountMigrationManager;
class QXmppAtmManager;
class QXmppEncryptedFileSharingProvider;
class QXmppFileSharingManager;
class QXmppHttpFileSharingProvider;
class QXmppMovedManager;
class QXmppRegistrationManager;
class QXmppRosterManager;
class QXmppVCardManager;
class QXmppVersionManager;

/**
 * The ClientWorker is used as a QObject-based worker on the ClientThread.
 */
class ClientWorker : public QObject
{
    Q_OBJECT
    Q_PROPERTY(DiscoveryManager *discoveryManager READ discoveryManager CONSTANT)

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
        RegistrationUnsupported,
        // The server requires the client to confirm an email message that the server sent to it.
        EmailConfirmationRequired,
        UnknownError,
    };
    Q_ENUM(ConnectionError)

    struct Caches {
        Caches(QObject *parent = nullptr);

        Settings *settings;
        VCardCache *vCardCache;
        PresenceCache *presenceCache;
        RosterModel *rosterModel;
        AvatarFileStorage *avatarStorage;
        ServerFeaturesCache *serverFeaturesCache;
    };

    /**
     * @param caches All caches running in the main thread for communication with the UI.
     * @param enableLogging If logging of the XMPP stream should be done.
     * @param parent Optional QObject-based parent.
     */
    ClientWorker(Caches *caches, Database *database, bool enableLogging, QObject *parent = nullptr);

    DiscoveryManager *discoveryManager() const
    {
        return m_discoveryManager;
    }

    Caches *caches() const
    {
        return m_caches;
    }

    QXmppClient *xmppClient() const
    {
        return m_client;
    }

    QXmppAccountMigrationManager *accountMigrationManager() const
    {
        return m_accountMigrationManager;
    }

    QXmppAtmManager *atmManager() const
    {
        return m_atmManager;
    }

    QXmppFileSharingManager *fileSharingManager() const
    {
        return m_fileSharingManager;
    }

    QXmppMovedManager *movedManager() const
    {
        return m_movedManager;
    }

    QXmppRegistrationManager *registrationManager() const
    {
        return m_registrationManager;
    }

    QXmppRosterManager *rosterManager() const
    {
        return m_rosterManager;
    }

    QXmppVCardManager *vCardManager() const
    {
        return m_vCardManager;
    }

    QXmppVersionManager *versionManager() const
    {
        return m_versionManager;
    }

    std::shared_ptr<QXmppEncryptedFileSharingProvider> encryptedHttpFileSharingProvider() const
    {
        return m_encryptedProvider;
    }

    std::shared_ptr<QXmppHttpFileSharingProvider> httpFileSharingProvider() const
    {
        return m_httpProvider;
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
    void startTask(const std::function<void()> &task);

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
    void logIn();

    /**
     * Connects to the server with a minimal configuration.
     *
     * Some additional configuration variables can be set by passing a configuration.
     *
     * @param config configuration with additional variables for connecting to the server
     * or nothing if only the minimal configuration should be used
     */
    void connectToServer(QXmppConfiguration config = QXmppConfiguration());

    /**
     * Logs out of the server if the client is not already logged out.
     *
     * @param isApplicationBeingClosed true if the application will be terminated directly after logging out, false otherwise
     */
    void logOut(bool isApplicationBeingClosed = false);

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
    void onConnectionError(const QXmppError &error);

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

    DiscoveryManager *m_discoveryManager;

    QXmppAccountMigrationManager *m_accountMigrationManager;
    QXmppAtmManager *m_atmManager;
    QXmppFileSharingManager *m_fileSharingManager;
    QXmppMovedManager *m_movedManager;
    QXmppRegistrationManager *m_registrationManager;
    QXmppRosterManager *m_rosterManager;
    QXmppVCardManager *m_vCardManager;
    QXmppVersionManager *m_versionManager;

    OmemoDb *m_omemoDb;

    std::shared_ptr<QXmppEncryptedFileSharingProvider> m_encryptedProvider;
    std::shared_ptr<QXmppHttpFileSharingProvider> m_httpProvider;

    QList<std::function<void()>> m_pendingTasks;
    uint m_activeTasks = 0;

    bool m_isFirstLoginAfterStart = true;
    bool m_isReconnecting = false;
    bool m_isDisconnecting = false;
    QXmppConfiguration m_configToBeUsedOnNextConnect;
};
