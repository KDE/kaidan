// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMutex>
#include <QObject>
// QXmpp
#include <QXmppGlobal.h>
#include <QXmppStanza.h>
// Kaidan
#include "Account.h"
#include "Encryption.h"
#include "Kaidan.h"

class Settings;
class VCardCache;
class QXmppSasl2UserAgent;

/**
 * This class manages account-related settings.
 */
class AccountManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Account account READ account NOTIFY accountChanged)
    Q_PROPERTY(quint16 portAutodetect READ portAutodetect CONSTANT)

public:
    enum class DeletionState {
        NotToBeDeleted = 1 << 0,
        ToBeDeletedFromClient = 1 << 1,
        ToBeDeletedFromServer = 1 << 2,
        DeletedFromServer = 1 << 3,
        ClientDisconnectedBeforeDeletionFromServer = 1 << 4,
    };
    Q_DECLARE_FLAGS(DeletionStates, DeletionState)

    static AccountManager *instance();

    AccountManager(Settings *settings, VCardCache *cache, QObject *parent = nullptr);

    Account account() const;
    Q_SIGNAL void accountChanged();

    bool hasNewAccount() const;

    // TODO: When multi account is there, Account must become a real QObject and those helpers goes into Account instead.
    Q_INVOKABLE void setAuthOnline(bool online);
    Q_INVOKABLE void setAuthJidResourcePrefix(const QString &prefix);
    Q_INVOKABLE void setAuthPassword(const QString &password);
    Q_INVOKABLE void setAuthCredentials(const QXmppCredentials &credentials);
    Q_INVOKABLE void setAuthHost(const QString &host);
    Q_INVOKABLE void setAuthPort(quint16 port);
    Q_INVOKABLE void setAuthTlsErrorsIgnored(bool enabled);
    Q_INVOKABLE void setAuthTlsRequirement(QXmppConfiguration::StreamSecurityMode mode);
    Q_INVOKABLE void setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility);
    Q_INVOKABLE void setUserAgentDeviceId(const QUuid &userAgentDeviceId);
    Q_INVOKABLE void setEncryption(Encryption::Enum encryption);
    Q_INVOKABLE void setAutomaticMediaDownloadsRule(Account::AutomaticMediaDownloadsRule rule);
    Q_INVOKABLE void setContactNotificationRule(const QString &jid, Account::ContactNotificationRule rule);
    Q_INVOKABLE void setGroupChatNotificationRule(const QString &jid, Account::GroupChatNotificationRule rule);
    Q_INVOKABLE void setGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled);
    Q_INVOKABLE void setGeoLocationMapService(const QString &jid, Account::GeoLocationMapService geoLocationMapService);

    Q_INVOKABLE void setNewAccount(const QString &jid, const QString &password, const QString &host = {}, quint16 port = PORT_AUTODETECT);
    Q_INVOKABLE void setNewAccountJid(const QString &jid);
    Q_INVOKABLE void setNewAccountPassword(const QString &password);
    Q_INVOKABLE void setNewAccountHost(const QString &host, quint16 port);
    Q_INVOKABLE void resetNewAccount();

    /**
     * Returns the port which indicates that no custom port is set.
     */
    quint16 portAutodetect() const;

    /**
     * Resets the custom connection settings.
     */
    Q_INVOKABLE void resetCustomConnectionSettings();

    /**
     * Returns whether there are enough credentials available to log in to the server.
     */
    Q_INVOKABLE bool hasEnoughCredentialsForLogin();

    /**
     * Returns the user agent of the current account (thread-safe).
     */
    QXmppSasl2UserAgent userAgent();

    /**
     * Loads all credentials and connection settings used to connect to the server.
     *
     * Emits connectionDataLoaded when done. Use hasEnoughCredentialsForLogin for the result.
     */
    Q_INVOKABLE void loadConnectionData();
    Q_SIGNAL void connectionDataLoaded();

    /**
     * Stores credentials and connection settings in the settings file.
     */
    void storeAccount();

    /**
     * Deletes the account data from the configuration file and database.
     */
    Q_INVOKABLE void deleteAccountFromClient();

    /**
     * Deletes the account data from the client and server.
     */
    Q_INVOKABLE void deleteAccountFromClientAndServer();

    /**
     * Called when the account is deleted from the server.
     */
    void handleAccountDeletedFromServer();

    /**
     * Called when the account could not be deleted from the server.
     *
     * @param error error of the failed account deletion
     */
    void handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error);
    Q_SIGNAL void accountDeletionFromClientAndServerFailed(const QString &errorMessage);

    bool handleConnected();
    void handleDisconnected();

Q_SIGNALS:
    /**
     * Emitted when there are no (correct) credentials to log in.
     */
    void credentialsNeeded();

private:
    /**
     * Removes an account.
     *
     * @param accountJid JID of the account being removed
     */
    void removeAccount(const QString &accountJid);

    mutable QMutex m_mutex;
    Settings *const m_settings;

    Account m_account;

    // Temporary account until stored to db
    // Use for new account setup or account registration.
    std::optional<Account> m_tmpAccount;

    // These variables are used for checking the state of an ongoing account deletion.
    DeletionStates m_deletionStates = DeletionState::NotToBeDeleted;

    static AccountManager *s_instance;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AccountManager::DeletionStates)
