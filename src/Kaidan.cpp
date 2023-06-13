// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Kaidan.h"

// Qt
#include <QGuiApplication>
#include <QSettings>
#include <QStringBuilder>
#include <QThread>
#include <QTimer>
// QXmpp
#include "qxmpp-exts/QXmppUri.h"
// Kaidan
#include "AccountManager.h"
#include "AtmManager.h"
#include "AvatarFileStorage.h"
#include "CredentialsValidator.h"
#include "Database.h"
#include "Globals.h"
#include "MessageDb.h"
#include "Notifications.h"
#include "RosterDb.h"
#include "FileSharingController.h"

Kaidan *Kaidan::s_instance;

Kaidan::Kaidan(bool enableLogging, QObject *parent)
	: QObject(parent)
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	m_notifications = new Notifications(this);

	// database
	m_database = new Database(this);
	m_msgDb = new MessageDb(m_database, this);
	m_rosterDb = new RosterDb(m_database, this);

	// caches
	m_caches = new ClientWorker::Caches(this);
	// Connect the avatar changed signal of the avatarStorage with the NOTIFY signal
	// of the Q_PROPERTY for the avatar storage (so all avatars are updated in QML)
	connect(m_caches->avatarStorage, &AvatarFileStorage::avatarIdsChanged, this, &Kaidan::avatarStorageChanged);

	// create xmpp thread
	m_cltThrd = new QThread();
	m_cltThrd->setObjectName("XmppClient");

	m_client = new ClientWorker(m_caches, m_database, enableLogging);
	m_client->moveToThread(m_cltThrd);

	connect(AccountManager::instance(), &AccountManager::credentialsNeeded, this, &Kaidan::credentialsNeeded);

	connect(m_client, &ClientWorker::loggedInWithNewCredentials, this, &Kaidan::openChatViewRequested);
	connect(m_client, &ClientWorker::connectionStateChanged, this, &Kaidan::setConnectionState);
	connect(m_client, &ClientWorker::connectionErrorChanged, this, &Kaidan::setConnectionError);

	m_cltThrd->start();

	// create controllers
	m_fileSharingController = std::make_unique<FileSharingController>(m_client->xmppClient());

	// Log out of the server when the application window is closed.
	connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this]() {
		emit logOutRequested(true);
	});
}

Kaidan::~Kaidan()
{
	s_instance = nullptr;
}

QString Kaidan::connectionStateText() const
{
	switch (m_connectionState) {
	case Enums::ConnectionState::StateConnected:
		return tr("Online");
	case Enums::ConnectionState::StateConnecting:
		return tr("Connecting…");
	case Enums::ConnectionState::StateDisconnected:
		return tr("Offline");
	}
	return {};
}

void Kaidan::setConnectionState(Enums::ConnectionState connectionState)
{
	if (m_connectionState != connectionState) {
		m_connectionState = connectionState;
		emit connectionStateChanged();

		// Open the possibly cached URI when connected.
		// This is needed because the XMPP URIs can't be opened when Kaidan is not connected.
		if (m_connectionState == ConnectionState::StateConnected && !m_openUriCache.isEmpty()) {
			// delay is needed because sometimes the RosterPage needs to be loaded first
			QTimer::singleShot(300, this, [this] {
				emit xmppUriReceived(m_openUriCache);
				m_openUriCache = "";
			});
		}
	}
}

void Kaidan::setConnectionError(ClientWorker::ConnectionError error)
{
	if (error != m_connectionError) {
		m_connectionError = error;
		emit connectionErrorChanged();
	}
}

void Kaidan::addOpenUri(const QString &uri)
{
	// Do not open XMPP URIs for group chats (e.g., "xmpp:kaidan@muc.kaidan.im?join") as long as Kaidan does not support that.
	if (!QXmppUri::isXmppUri(uri) || QXmppUri(uri).action() == QXmppUri::Join)
		return;

	if (m_connectionState == ConnectionState::StateConnected) {
		emit xmppUriReceived(uri);
	} else {
		emit passiveNotificationRequested(tr("The link will be opened after you have connected."));
		m_openUriCache = uri;
	}
}

quint8 Kaidan::logInByUri(const QString &uri)
{
	if (!QXmppUri::isXmppUri(uri)) {
		return quint8(LoginByUriState::InvalidLoginUri);
	}

	QXmppUri parsedUri(uri);

	if (!CredentialsValidator::isUserJidValid(parsedUri.jid())) {
		return quint8(LoginByUriState::InvalidLoginUri);
	}

	AccountManager::instance()->setJid(parsedUri.jid());

	if (parsedUri.action() != QXmppUri::Login || !CredentialsValidator::isPasswordValid(parsedUri.password())) {
		return quint8(LoginByUriState::PasswordNeeded);
	}

	// Connect with the extracted credentials.
	AccountManager::instance()->setPassword(parsedUri.password());
	logIn();
	return quint8(LoginByUriState::Connecting);
}

Kaidan::TrustDecisionByUriResult Kaidan::makeTrustDecisionsByUri(const QString &uri, const QString &expectedJid)
{
	if (QXmppUri::isXmppUri(uri)) {
		auto parsedUri = QXmppUri(uri);

		if (expectedJid.isEmpty() || parsedUri.jid() == expectedJid) {
			if (parsedUri.action() != QXmppUri::TrustMessage || parsedUri.encryption().isEmpty() || (parsedUri.trustedKeysIds().isEmpty() && parsedUri.distrustedKeysIds().isEmpty())) {
				return InvalidUri;
			}

			runOnThread(m_client->atmManager(), [uri = std::move(parsedUri)]() {
				Kaidan::instance()->client()->atmManager()->makeTrustDecisions(uri);
			});
			return MakingTrustDecisions;
		} else {
			return JidUnexpected;
		}
	}

	return InvalidUri;
}

#ifdef NDEBUG
QString configFileBaseName()
{
	return QStringLiteral(APPLICATION_NAME);
}

QStringList oldDatabaseFilenames()
{
	return { QStringLiteral("messages.sqlite3") };
}

QString databaseFilename()
{
	return QStringLiteral(DB_FILE_BASE_NAME ".sqlite3");
}
#else
/**
 * Returns the name of the application profile usable as a suffix.
 *
 * The profile name is the value of the environment variable "KAIDAN_PROFILE".
 * Only available in non-debug builds.
 *
 * @return the application profile name
 */
QString applicationProfileSuffix()
{
	static const auto profileSuffix = []() -> QString {
		const auto profile = qEnvironmentVariable("KAIDAN_PROFILE");
		if (!profile.isEmpty()) {
			return u'-' + profile;
		}
		return {};
	}();
	return profileSuffix;
}

QString configFileBaseName()
{
	return u"" APPLICATION_NAME % applicationProfileSuffix();
}

QStringList oldDatabaseFilenames()
{
	return {u"messages" % applicationProfileSuffix() % u".sqlite3"};
}

QString databaseFilename()
{
	return u"" DB_FILE_BASE_NAME % applicationProfileSuffix() % u".sqlite3";
}
#endif
