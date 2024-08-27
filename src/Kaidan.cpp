// SPDX-FileCopyrightText: 2016 geobra <s.g.b@gmx.de>
// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Kaidan.h"

// Qt
#include <QFile>
#include <QGuiApplication>
#include <QSettings>
#include <QStringBuilder>
#include <QThread>
#include <QTimer>
#include <QWaitCondition>
// QXmpp
#include "qxmpp-exts/QXmppUri.h"
// Kaidan
#include "AccountDb.h"
#include "AccountManager.h"
#include "AtmManager.h"
#include "AvatarFileStorage.h"
#include "Blocking.h"
#include "ChatController.h"
#include "ChatHintModel.h"
#include "CredentialsValidator.h"
#include "Database.h"
#include "EncryptionController.h"
#include "FileSharingController.h"
#include "Globals.h"
#include "GroupChatUserDb.h"
#include "GroupChatController.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "MessageModel.h"
#include "Notifications.h"
#include "RosterDb.h"
#include "RosterModel.h"
#include "Settings.h"

Kaidan *Kaidan::s_instance;

/**
 * Allows to run a function and waits for the thread to be started before returning to the calling
 * thread.
 *
 * This is needed because QXmppMigrationManager expects the classes of the import/export functions
 * to be registered while they are running in the right/final thread.
 */
class ClientThread : public QThread
{
public:
	explicit ClientThread(std::future<void> &&future) : m_future(std::move(future))
	{
	}

	~ClientThread() override
	{
		requestInterruption();
		quit();
		wait();
	}

	void waitForStarted(QThread::Priority priority = InheritPriority)
	{
		if (isRunning()) {
			return;
		}

		QMutexLocker locker(&m_mutex);
		QThread::start(priority);
		m_condition.wait(&m_mutex);
	}

	template<typename Function, typename... Args>
	static ClientThread *create(Function &&f, Args &&...args)
	{
		using DecayedFunction = typename std::decay<Function>::type;
		auto threadFunction = [f = static_cast<DecayedFunction>(std::forward<Function>(f))](
					      auto &&...largs) mutable -> void {
			(void) std::invoke(std::move(f), std::forward<decltype(largs)>(largs)...);
		};
		return new ClientThread(std::move(std::async(
			std::launch::deferred, std::move(threadFunction), std::forward<Args>(args)...)));
	}

protected:
	void run() override
	{
		m_future.get();
		m_condition.wakeOne();
		QThread::exec();
	}

private:
	QWaitCondition m_condition;
	QMutex m_mutex;
	std::future<void> m_future;
};

Kaidan::Kaidan(bool enableLogging, QObject *parent)
	: QObject(parent)
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	m_notifications = new Notifications(this);

	// database
	m_database = new Database(this);
	m_accountDb = new AccountDb(m_database, this);
	m_msgDb = new MessageDb(m_database, this);
	m_rosterDb = new RosterDb(m_database, this);
	m_groupChatUserDb = new GroupChatUserDb(m_database, this);

	// caches
	m_caches = new ClientWorker::Caches(this);
	// Connect the avatar changed signal of the avatarStorage with the NOTIFY signal
	// of the Q_PROPERTY for the avatar storage (so all avatars are updated in QML)
	connect(m_caches->avatarStorage, &AvatarFileStorage::avatarIdsChanged, this, &Kaidan::avatarStorageChanged);

	// create xmpp thread
	m_cltThrd = ClientThread::create([&] {
		m_client = new ClientWorker(m_caches, m_database, enableLogging);
	});
	m_cltThrd->setObjectName("XmppClient");
	m_cltThrd->waitForStarted();

	connect(AccountManager::instance(), &AccountManager::credentialsNeeded, this, &Kaidan::credentialsNeeded);
	connect(m_client, &ClientWorker::loggedInWithNewCredentials, this, &Kaidan::openChatViewRequested);
	connect(m_client, &ClientWorker::connectionStateChanged, this, &Kaidan::setConnectionState);
	connect(m_client, &ClientWorker::connectionErrorChanged, this, &Kaidan::setConnectionError);

	// create controllers
	m_blockingController = std::make_unique<BlockingController>(m_database);
	m_chatController = new ChatController(this);
	m_encryptionController = new EncryptionController(this);
	m_fileSharingController = std::make_unique<FileSharingController>(m_client->xmppClient());
	m_groupChatController = new GroupChatController(this);
	m_messageController = new MessageController(this);

	m_messageModel = new MessageModel(this);
	m_chatHintModel = new ChatHintModel(this);

	initializeAccountMigration();

	// Log out of the server when the application window is closed.
	connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this]() {
		Q_EMIT logOutRequested(true);
	});

	connect(m_msgDb, &MessageDb::messageAdded, this, [this](const Message &message, MessageOrigin origin) {
		if (origin != MessageOrigin::UserInput && !message.files.isEmpty()) {
			if (const auto item = RosterModel::instance()->findItem(message.chatJid)) {
				const auto contactRule = item->automaticMediaDownloadsRule;

				const auto effectiveRule = [this, contactRule]() -> Account::AutomaticMediaDownloadsRule {
					switch (contactRule) {
					case RosterItem::AutomaticMediaDownloadsRule::Account:
						return settings()->automaticMediaDownloadsRule();
					case RosterItem::AutomaticMediaDownloadsRule::Never:
						return Account::AutomaticMediaDownloadsRule::Never;
					case RosterItem::AutomaticMediaDownloadsRule::Always:
						return Account::AutomaticMediaDownloadsRule::Always;
					}

					Q_UNREACHABLE();
				}();

				const auto automaticDownloadDesired = [effectiveRule, &message]() -> bool {
					switch (effectiveRule) {
					case Account::AutomaticMediaDownloadsRule::Never:
						return false;
					case Account::AutomaticMediaDownloadsRule::PresenceOnly:
						return message.isOwn || RosterModel::instance()->isPresenceSubscribedByItem(message.accountJid, message.chatJid);
					case Account::AutomaticMediaDownloadsRule::Always:
						return true;
					}

					Q_UNREACHABLE();
				}();

				if (automaticDownloadDesired) {
					for (const auto &file : message.files) {
						if (file.localFilePath.isEmpty() || !QFile::exists(file.localFilePath)) {
							m_fileSharingController->downloadFile(message.id, file);
						}
					}
				}
			}
		}
	});
}

Kaidan::~Kaidan()
{
	m_cltThrd->quit();
	m_cltThrd->wait();
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
		Q_EMIT connectionStateChanged();

		// Open the possibly cached URI when connected.
		// This is needed because the XMPP URIs can't be opened when Kaidan is not connected.
		if (m_connectionState == ConnectionState::StateConnected && !m_openUriCache.isEmpty()) {
			// delay is needed because sometimes the RosterPage needs to be loaded first
			QTimer::singleShot(300, this, [this] {
				Q_EMIT xmppUriReceived(m_openUriCache);
				m_openUriCache.clear();
			});
		}
	}
}

void Kaidan::setConnectionError(ClientWorker::ConnectionError error)
{
	// For displaying errors to the user, every new error (even if it is the same as before) must be
	// emitted.
	// Thus, the error is not checked for a change but simply emitted.
	m_connectionError = error;
	Q_EMIT connectionErrorChanged();
}

void Kaidan::initializeAccountMigration()
{
	auto migrationManager = m_client->accountMigrationManager();

	connect(migrationManager, &AccountMigrationManager::migrationStateChanged, this, &Kaidan::accountMigrationStateChanged);
	connect(migrationManager, &AccountMigrationManager::busyChanged, this, &Kaidan::accountBusyChanged);
	connect(migrationManager, &AccountMigrationManager::errorOccurred, this, &Kaidan::accountErrorOccurred);

	connect(migrationManager, &AccountMigrationManager::migrationStateChanged, this, [this, migrationManager](AccountMigrationManager::MigrationState state) {
		switch (state) {
		case AccountMigrationManager::MigrationState::Idle:
		case AccountMigrationManager::MigrationState::Exporting:
		case AccountMigrationManager::MigrationState::Importing:
			break;
		case AccountMigrationManager::MigrationState::Started:
		case AccountMigrationManager::MigrationState::Finished:
			migrationManager->continueMigration();
			break;
		case AccountMigrationManager::MigrationState::ChoosingNewAccount:
			Q_EMIT openStartPageRequested();
			break;
		}
	});

	connect(migrationManager, &AccountMigrationManager::errorOccurred, this, [this, migrationManager](const QString &error) {
		switch (migrationManager->migrationState()) {
		case AccountMigrationManager::MigrationState::Idle:
		case AccountMigrationManager::MigrationState::Exporting:
		case AccountMigrationManager::MigrationState::ChoosingNewAccount:
		case AccountMigrationManager::MigrationState::Importing:
			Q_EMIT passiveNotificationRequested(error);
			break;
		case AccountMigrationManager::MigrationState::Started:
		case AccountMigrationManager::MigrationState::Finished:
			break;
		}
	});
}

void Kaidan::addOpenUri(const QString &uri)
{
	// Do not open XMPP URIs for group chats (e.g., "xmpp:kaidan@muc.kaidan.im?join") as long as Kaidan does not support that.
	if (!QXmppUri::isXmppUri(uri) || QXmppUri(uri).action() == QXmppUri::Join)
		return;

	if (m_connectionState == ConnectionState::StateConnected) {
		Q_EMIT xmppUriReceived(uri);
	} else {
		Q_EMIT passiveNotificationRequested(tr("The link will be opened after you have connected."));
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
				Kaidan::instance()->client()->atmManager()->makeTrustDecisionsByUri(uri);
			});
			return MakingTrustDecisions;
		} else {
			return JidUnexpected;
		}
	}

	return InvalidUri;
}

void Kaidan::startAccountMigration()
{
	QMetaObject::invokeMethod(m_client->accountMigrationManager(), &AccountMigrationManager::startMigration);
}

void Kaidan::continueAccountMigration(const QVariant &userData)
{
	QMetaObject::invokeMethod(m_client->accountMigrationManager(), [this, userData]() {
		m_client->accountMigrationManager()->continueMigration(userData);
	});
}

void Kaidan::cancelAccountMigration()
{
	QMetaObject::invokeMethod(m_client->accountMigrationManager(), &AccountMigrationManager::cancelMigration);
}

bool Kaidan::testAccountMigrationState(AccountMigrationManager::MigrationState state)
{
	bool ret;
	if (QMetaObject::invokeMethod(
		    m_client->accountMigrationManager(),
		    [this, state]() {
			    return m_client->accountMigrationManager()->migrationState() == state;
		    },
		    Qt::BlockingQueuedConnection,
		    &ret)) {
		return ret;
	}

	return false;
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
