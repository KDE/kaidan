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

class Settings;
class VCardCache;
class QXmppSasl2UserAgent;

/**
 * This class manages account-related settings.
 */
class AccountManager : public QObject
{
    Q_OBJECT

    Q_PROPERTY(const Account &account READ account NOTIFY accountChanged)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
    Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
    Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
    Q_PROPERTY(quint16 portAutodetect READ portAutodetect CONSTANT)
    Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)

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

    const Account &account() const;
    Q_SIGNAL void accountChanged();

    Q_INVOKABLE void setContactNotificationRule(const QString &jid, Account::ContactNotificationRule rule);
    Q_INVOKABLE void setGroupChatNotificationRule(const QString &jid, Account::GroupChatNotificationRule rule);
    Q_INVOKABLE void setGeoLocationMapPreviewEnabled(const QString &jid, bool geoLocationMapPreviewEnabled);
    Q_INVOKABLE void setGeoLocationMapService(const QString &jid, Account::GeoLocationMapService geoLocationMapService);

    /**
     * Returns the bare JID of the account.
     *
     * This method is thread-safe.
     */
    QString jid();

    /**
     * Sets the bare JID of the account.
     *
     * This method is thread-safe.
     *
     * @param jid bare JID of the account
     */
    void setJid(const QString &jid);

    /**
     * Returns the resource part of the account's full JID.
     */
    QString jidResource() const;

    /**
     * Sets the prefix of the account's full JID's resource part.
     *
     * The resource prefix is used to create the complete resource. The resource is created by
     * appending a dot and random alphanumeric characters.
     *
     * Example:
     *  Full JID: alice@example.org/Kaidan.DzF9
     *  Resource prefix: Kaidan
     *
     * @param jidResourcePrefix prefix of the account's full JID's resource part
     */
    void setJidResourcePrefix(const QString &jidResourcePrefix);

    /**
     * Returns the password of the account.
     *
     * This method is thread-safe.
     */
    QString password();

    /**
     * Sets the password of the account.
     *
     * This method is thread-safe.
     *
     * @param password password of the account
     */
    void setPassword(const QString &password);

    /**
     * Returns the custom host.
     *
     * This method is thread-safe.
     *
     * @return the custom host or an empty string if no custom host is set
     */
    QString host();

    /**
     * Sets a custom host for connecting.
     *
     * This method is thread-safe.
     *
     * @param host host to connect to
     */
    void setHost(const QString &host);

    /**
     * Returns the custom port.
     *
     * This method is thread-safe.
     *
     * @return the custom port or portAutodetect if no custom port is set
     */
    quint16 port();

    /**
     * Sets a custom port for connecting.
     *
     * This method is thread-safe.
     *
     * @param port port to connect to
     */
    void setPort(const quint16 port);

    /**
     * Returns the port which indicates that no custom port is set.
     */
    quint16 portAutodetect() const;

    /**
     * Returns the user's display name.
     */
    QString displayName();

    /**
     * Resets the custom connection settings.
     */
    Q_INVOKABLE void resetCustomConnectionSettings();

    /**
     * Provides a way to cache whether the current credentials are new to this client.
     *
     * The credentials are new to the client if they were not already in use. That is the case
     * after entering the credentials the first time to log in or on the first login after
     * registration.
     *
     * @return true if the credentials are new, otherwise false
     */
    bool hasNewCredentials() const;

    /**
     * Sets whether the current credentials are new to this client.
     *
     * @param hasNewCredentials true if the credentials are new, otherwise false
     */
    void setHasNewCredentials(bool hasNewCredentials);

    /**
     * Returns whether there are enough credentials available to log in to the server.
     */
    bool hasEnoughCredentialsForLogin();

    /**
     * Provides a way to cache whether the current connection settings are new to this client.
     *
     * The connections settings are new to the client if they were not already in use during the
     * previous login.
     *
     * @return whether the connection settings are new
     */
    bool hasNewConnectionSettings() const;

    /**
     * Sets whether the current connection settings are new to this client.
     *
     * @param hasNewConnectionSettings whether the settings are new
     */
    void setHasNewConnectionSettings(bool hasNewConnectionSettings);

    /**
     * Returns the user agent of the current account (thread-safe).
     */
    QXmppSasl2UserAgent userAgent();

    /**
     * Loads all credentials and connection settings used to connect to the server.
     *
     * @return true if the credentials could be loaded, otherwise false
     */
    Q_INVOKABLE bool loadConnectionData();

    /**
     * Stores the currently set JID in the settings file.
     */
    void storeJid();

    /**
     * Stores the currently set password in the settings file.
     */
    void storePassword();

    /**
     * Stores the currently set custom host and port in the settings file.
     *
     * If a custom connection setting is not set, it is removed from the settings file.
     */
    void storeCustomConnectionSettings();

    /**
     * Stores credentials and connection settings in the settings file.
     */
    void storeConnectionData();

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
     * Emitted when the JID changed.
     */
    void jidChanged();

    /**
     * Emitted when the password changed.
     */
    void passwordChanged();

    /**
     * Emitted when the custom host changed.
     */
    void hostChanged();

    /**
     * Emitted when the custom port changed.
     */
    void portChanged();

    /**
     * Emitted when the user's display name changed.
     */
    void displayNameChanged();

    /**
     * Emitted when there are no (correct) credentials to log in.
     */
    void credentialsNeeded();

private:
    /**
     * Generates the JID's resource part with the set JID resource prefix and a suffix
     * consisting of a dot followed by random alphanumeric characters.
     *
     * @param numberOfRandomSuffixCharacters number of random alphanumeric characters the
     * suffix should consist of after the dot
     */
    QString generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters = 4) const;

    /**
     * Removes an account.
     *
     * @param accountJid JID of the account being removed
     */
    void removeAccount(const QString &accountJid);

    QMutex m_mutex;
    Settings *const m_settings;

    Account m_account;

    QString m_jid;
    QString m_jidResourcePrefix;
    QString m_jidResource;
    QString m_password;
    QString m_host;
    quint16 m_port;

    bool m_hasNewCredentials;
    bool m_hasNewConnectionSettings;

    // These variables are used for checking the state of an ongoing account deletion.
    DeletionStates m_deletionStates = DeletionState::NotToBeDeleted;

    static AccountManager *s_instance;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(AccountManager::DeletionStates)
