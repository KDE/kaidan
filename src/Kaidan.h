// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2018 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "ClientWorker.h"

class QSize;
class Database;
class DataFormModel;
class Notifications;
class RosterDb;
class MessageDb;
class FileSharingController;

/**
 * @class Kaidan Kaidan's Back-End Class
 *
 * @brief This class will initiate the complete back-end, including the @see Database
 * connection, viewing models (@see MessageModel, @see RosterModel), etc.
 *
 * This class will run in the main thread, the XMPP connection and the database managers
 * run in other threads.
 */
class Kaidan : public QObject
{
	Q_OBJECT

	Q_PROPERTY(ClientWorker* client READ client CONSTANT)
	Q_PROPERTY(FileSharingController *fileSharingController READ fileSharingController CONSTANT)
	Q_PROPERTY(AvatarFileStorage* avatarStorage READ avatarStorage NOTIFY avatarStorageChanged)
	Q_PROPERTY(ServerFeaturesCache* serverFeaturesCache READ serverFeaturesCache CONSTANT)
	Q_PROPERTY(Settings* settings READ settings CONSTANT)
	Q_PROPERTY(quint8 connectionState READ connectionState NOTIFY connectionStateChanged)
	Q_PROPERTY(QString connectionStateText READ connectionStateText NOTIFY connectionStateChanged)
	Q_PROPERTY(quint8 connectionError READ connectionError NOTIFY connectionErrorChanged)

public:
	/**
	 * Result for making trust decisions by an XMPP URI specifying how the URI
	 * is used
	 */
	enum TrustDecisionByUriResult {
		MakingTrustDecisions,   ///< The trust decisions are being made.
		JidUnexpected,          ///< The URI's JID is not the expected one.
		InvalidUri              ///< The URI cannot be used for trust decisions.
	};
	Q_ENUM(TrustDecisionByUriResult)

	/**
	 * State which specifies in which way a password is shown on the account transfer page
	 */
	enum PasswordVisibility {
		PasswordVisible, ///< The password is included in the QR code and shown as plain text.
		PasswordVisibleQrOnly, ///< The password is included in the QR code but not shown as plain text.
		PasswordInvisible ///< The password is neither included in the QR code nor shown as plain text.
	};
	Q_ENUM(PasswordVisibility)

	static Kaidan *instance() { return s_instance; }

	/**
	 * Constructs Kaidan's main object and initializes all components / threads.
	 *
	 * @param enableLogging true to enable logging, otherwise false
	 */
	Kaidan(bool enableLogging = true, QObject *parent = nullptr);

	~Kaidan();

	/**
	 * Connects to the XMPP server and logs in to it.
	 *
	 * The username and password are retrieved from the settings file.
	 */
	Q_INVOKABLE void logIn()
	{
		emit logInRequested();
	}

	/**
	 * Connects to the server and requests a data form for account registration.
	 */
	Q_INVOKABLE void requestRegistrationForm()
	{
		emit registrationFormRequested();
	}

	/**
	 * Logs out of the XMPP server.
	 *
	 * This disconnects the client from the server.
	 * When disconnected, the connectionStateChanged signal is emitted.
	 */
	Q_INVOKABLE void logOut()
	{
		emit logOutRequested();
	}

	/**
	 * Returns the current ConnectionState
	 */
	quint8 connectionState() const
	{
		return quint8(m_connectionState);
	}

	QString connectionStateText() const;

	/**
	 * Returns the last connection error.
	 */
	quint8 connectionError() const { return quint8(m_connectionError); }

	ClientWorker *client() const { return m_client; }
	FileSharingController *fileSharingController() const { return m_fileSharingController.get(); }
	AvatarFileStorage *avatarStorage() const { return m_caches->avatarStorage; }
	ServerFeaturesCache *serverFeaturesCache() const { return m_caches->serverFeaturesCache; }
	VCardCache *vCardCache() const { return m_caches->vCardCache; }
	Settings *settings() const { return m_caches->settings; }
	Database *database() const { return m_database; }

	/**
	 * Adds XMPP URI to open as soon as possible
	 */
	void addOpenUri(const QString &uri);

	/**
	 * Connects to the server by the parsed credentials (bare JID and
	 * password) from a given XMPP URI (e.g. from scanning a QR code) such
	 * as "xmpp:user@example.org?login;password=abc"
	 *
	 * The URI is used in the following cases.
	 *
	 * Login attempt (LoginByUriState::Connecting is returned):
	 *	xmpp:user@example.org?login;password=abc
	 *
	 * Pre-fill of JID for opening login page (LoginByUriState::PasswordNeeded is returned):
	 *	xmpp:user@example.org?login;password=
	 *	xmpp:user@example.org?login;password
	 *	xmpp:user@example.org?login;
	 *	xmpp:user@example.org?login
	 *	xmpp:user@example.org?
	 *	xmpp:user@example.org
	 *
	 * In all other cases, LoginByUriState::InvalidLoginUri is returned.
	 *
	 * @param uri string which can be an XMPP login URI
	 *
	 * @return the state which specifies how the XMPP login URI was used
	 */
	Q_INVOKABLE quint8 logInByUri(const QString &uri);

	/**
	 * Authenticates or distrusts end-to-end encryption keys by a given XMPP URI
	 * (e.g., from a scanned QR code).
	 *
	 * Only if the URI's JID matches the expectedJid or no expectedJid is
	 * passed, the trust decision is made.
	 *
	 * @param uri string which can be an XMPP Trust Message URI
	 * @param expectedJid JID of the key owner whose keys are expected to be
	 *        authenticated or none to allow all JIDs
	 */
	Q_INVOKABLE Kaidan::TrustDecisionByUriResult makeTrustDecisionsByUri(const QString &uri, const QString &expectedJid = {});

signals:
	/**
	 * Emitted when a data form for registration is received from the server.
	 *
	 * @param dataFormModel received model for the registration data form
	 */
	void registrationFormReceived(DataFormModel *dataFormModel);

	/**
	 * Emitted when an out-of-band URL for registration is received from the
	 * server.
	 *
	 * @param outOfBandUrl URL used for out-of-band registration
	 */
	void registrationOutOfBandUrlReceived(const QString &outOfBandUrl);

	/**
	 * Emitted to request a registration form from the server which is set as the
	 * currently used JID.
	 */
	void registrationFormRequested();

	/**
	 * Emitted when the account registration failed.
	 *
	 * @param error received error
	 * @param errorMessage message describing the error
	 */
	void registrationFailed(quint8 error, const QString &errorMessage);

	/**
	 * Emitted to log in to the server with the set credentials.
	 */
	void logInRequested();

	/**
	 * Emitted to log out of the server.
	 *
	 * @param isApplicationBeingClosed true if the application will be terminated directly after logging out, false otherwise
	 */
	void logOutRequested(bool isApplicationBeingClosed = false);

	void avatarStorageChanged();

	/**
	 * Emitted, when the client's connection state has changed (e.g. when
	 * successfully connected or when disconnected)
	 */
	void connectionStateChanged();

	/**
	 * Emitted when the connection error changed.
	 *
	 * That is the case when the client failed to connect or it succeeded to connect after an error.
	 */
	void connectionErrorChanged();

	/**
	 * Emitted when there are no (correct) credentials and new ones are needed.
	 *
	 * The client will be in disconnected state when this is emitted.
	 */
	void credentialsNeeded();

	/**
	 * Emitted when an authenticated connection to the server is established with new credentials for the first time.
	 *
	 * The client will be in connected state when this is emitted.
	 */
	void loggedInWithNewCredentials();

	/**
	 * Raises the window to the foreground so that it is on top of all other windows.
	 */
	void raiseWindowRequested();

	/**
	 * Opens the view with the roster and empty chat page.
	 */
	void openChatViewRequested();

	/**
	 * Opens the chat page for a given chat.
	 *
	 * @param accountJid JID of the account for that the chat page is opened
	 * @param chatJid JID of the chat for that the chat page is opened
	 */
	void openChatPageRequested(const QString &accountJid, const QString &chatJid);

	/**
	 * Emitted when the removal state of the password on the account transfer page changed.
	 */
	void passwordVisibilityChanged();

	/**
	 * Show passive notification
	 */
	void passiveNotificationRequested(QString text);

	/**
	 * XMPP URI received
	 *
	 * Is called when Kaidan was used to open an XMPP URI (i.e. 'xmpp:kaidan@muc.kaidan.im?join')
	 */
	void xmppUriReceived(QString uri);

	/**
	 * Emitted when changing of the user's password finished succfessully.
	 */
	void passwordChangeSucceeded();

	/**
	 * Emitted when changing the user's password failed.
	 *
	 * @param errorMessage message describing the error
	 */
	void passwordChangeFailed(const QString &errorMessage);

	/**
	 * Emitted when changing of the user's avatar finished succfessully.
	 */
	void avatarChangeSucceeded();

	/**
	 * Deletes the account data from the client and server.
	 */
	void deleteAccountFromClientAndServer();

	/**
	 * Deletes the account data from the configuration file and database.
	 */
	void deleteAccountFromClient();

public:
	/**
	 * Set current connection state
	 */
	Q_INVOKABLE void setConnectionState(Enums::ConnectionState connectionState);

	/**
	 * Sets a new connection error.
	 */
	Q_INVOKABLE void setConnectionError(ClientWorker::ConnectionError error);

	/**
	 * Receives messages from another instance of the application
	 */
	Q_INVOKABLE void receiveMessage(quint32, const QByteArray &msg)
	{
		// currently we only send XMPP URIs
		addOpenUri(msg);
	}

private:
	Notifications *m_notifications;
	Database *m_database;
	MessageDb *m_msgDb;
	RosterDb *m_rosterDb;
	QThread *m_cltThrd;
	ClientWorker::Caches *m_caches;
	ClientWorker *m_client;
	std::unique_ptr<FileSharingController> m_fileSharingController;

	QString m_openUriCache;
	Enums::ConnectionState m_connectionState = Enums::ConnectionState::StateDisconnected;
	ClientWorker::ConnectionError m_connectionError = ClientWorker::NoError;

	static Kaidan *s_instance;
};

/**
 * Returns the name of the configuration file without its file extension.
 *
 * @return the config file name without extension
 */
QString configFileBaseName();

/**
 * Returns the old names of the database file.
 *
 * @return the old database file names
 */
QStringList oldDatabaseFilenames();

/**
 * Returns the name of the database file.
 *
 * @return the database file name
 */
QString databaseFilename();
