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

// Qt
class QNetworkAccessManager;
// QXmpp
class QXmppAccountMigrationManager;
class QXmppAtmManager;
class QXmppBlockingManager;
class QXmppCallManager;
class QXmppDiscoveryManager;
class QXmppEncryptedFileSharingProvider;
class QXmppFileSharingManager;
class QXmppHttpFileSharingProvider;
class QXmppHttpUploadManager;
class QXmppJingleMessageInitiationManager;
class QXmppMamManager;
class QXmppMessageReceiptManager;
class QXmppMixManager;
class QXmppMovedManager;
class QXmppOmemoManager;
class QXmppRegistrationManager;
class QXmppRosterManager;
class QXmppVCardManager;
class QXmppVersionManager;
// Kaidan
class AccountSettings;
class AtmController;
class EncryptionController;
class MessageController;
class PresenceCache;
class RegistrationController;

class ClientController : public QObject
{
    Q_OBJECT

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

    ClientController(AccountSettings *accountSettings, QObject *parent = nullptr);

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

    QXmppBlockingManager *blockingManager() const
    {
        return m_blockingManager;
    }

    QXmppCallManager *callManager() const
    {
        return m_callManager;
    }

    QXmppDiscoveryManager *discoveryManager() const
    {
        return m_discoveryManager;
    }

    QXmppFileSharingManager *fileSharingManager() const
    {
        return m_fileSharingManager;
    }

    QXmppJingleMessageInitiationManager *jmiManager() const
    {
        return m_jmiManager;
    }

    QXmppMamManager *mamManager() const
    {
        return m_mamManager;
    }

    QXmppMessageReceiptManager *messageReceiptManager() const
    {
        return m_messageReceiptManager;
    }

    QXmppMixManager *mixManager() const
    {
        return m_mixManager;
    }

    QXmppMovedManager *movedManager() const
    {
        return m_movedManager;
    }

    QXmppOmemoManager *omemoManager() const
    {
        return m_omemoManager;
    }

    QXmppRegistrationManager *registrationManager() const
    {
        return m_registrationManager;
    }

    QXmppRosterManager *rosterManager() const
    {
        return m_rosterManager;
    }

    QXmppHttpUploadManager *uploadManager() const
    {
        return m_uploadManager;
    }

    QXmppVCardManager *vCardManager() const
    {
        return m_vCardManager;
    }

    QXmppVersionManager *versionManager() const
    {
        return m_versionManager;
    }

    std::shared_ptr<QXmppHttpFileSharingProvider> httpFileSharingProvider() const
    {
        return m_httpProvider;
    }

    std::shared_ptr<QXmppEncryptedFileSharingProvider> encryptedHttpFileSharingProvider() const
    {
        return m_encryptedProvider;
    }

    void initialize(AtmController *atmController,
                    EncryptionController *encryptionController,
                    MessageController *messageController,
                    RegistrationController *registrationController,
                    PresenceCache *presenceCache);

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
    Q_SIGNAL void connectionErrorChanged(ClientController::ConnectionError error);

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

    AccountSettings *const m_accountSettings;

    AtmController *m_atmController;
    EncryptionController *m_encryptionController;
    MessageController *m_messageController;
    RegistrationController *m_registrationController;
    PresenceCache *m_presenceCache;

    QNetworkAccessManager *const m_networkAccessManager;

    QXmppClient *const m_client;

    QXmppAccountMigrationManager *const m_accountMigrationManager;
    QXmppAtmManager *const m_atmManager;
    QXmppBlockingManager *const m_blockingManager;
    QXmppCallManager *const m_callManager;
    QXmppDiscoveryManager *const m_discoveryManager;
    QXmppFileSharingManager *const m_fileSharingManager;
    QXmppJingleMessageInitiationManager *const m_jmiManager;
    QXmppMamManager *const m_mamManager;
    QXmppMessageReceiptManager *const m_messageReceiptManager;
    QXmppRosterManager *const m_rosterManager;
    QXmppMixManager *const m_mixManager;
    QXmppMovedManager *const m_movedManager;
    QXmppOmemoManager *const m_omemoManager;
    QXmppRegistrationManager *const m_registrationManager;
    QXmppHttpUploadManager *const m_uploadManager;
    QXmppVCardManager *const m_vCardManager;
    QXmppVersionManager *const m_versionManager;

    std::shared_ptr<QXmppHttpFileSharingProvider> m_httpProvider;
    std::shared_ptr<QXmppEncryptedFileSharingProvider> m_encryptedProvider;

    bool m_isReconnecting = false;
    bool m_isDisconnecting = false;
    QXmppConfiguration m_configToBeUsedOnNextConnect;
};
