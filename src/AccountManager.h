// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMutex>
#include <QObject>

class Settings;
class VCardCache;

/**
 * This class manages account-related settings.
 */
class AccountManager : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(QString password READ password WRITE setPassword NOTIFY passwordChanged)
	Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
	Q_PROPERTY(quint16 port READ port WRITE setPort NOTIFY portChanged)
	Q_PROPERTY(quint16 portAutodetect READ portAutodetect CONSTANT)
	Q_PROPERTY(QString displayName READ displayName NOTIFY displayNameChanged)

public:
	static AccountManager *instance();

	AccountManager(Settings *settings, VCardCache *cache, QObject *parent = nullptr);

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
	 * Deletes all credentials.
	 *
	 * Credentials stored in the settings file are also removed from it.
	 */
	void deleteCredentials();

	/**
	 * Deletes all account related settings.
	 *
	 * Settings stored in the settings file are also removed from it.
	 */
	void deleteSettings();

	/**
	 * Removes an account.
	 *
	 * @param accountJid JID of the account being removed
	 */
	void removeAccount(const QString &accountJid);

signals:
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

	/**
	 * Emitted to remove an account.
	 *
	 * @param accountJid JID of the account being removed
	 */
	void removeAccountRequested(const QString &accountJid);

private:
	/**
	 * Generates the JID's resource part with the set JID resource prefix and a suffix
	 * consisting of a dot followed by random alphanumeric characters.
	 *
	 * @param numberOfRandomSuffixCharacters number of random alphanumeric characters the
	 * suffix should consist of after the dot
	 */
	QString generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters = 4) const;

	QMutex m_mutex;
	Settings *m_settings;

	QString m_jid;
	QString m_jidResourcePrefix;
	QString m_jidResource;
	QString m_password;
	QString m_host;
	quint16 m_port;

	bool m_hasNewCredentials;
	bool m_hasNewConnectionSettings;

	static AccountManager *s_instance;
};
