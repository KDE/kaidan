// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountManager.h"

// Qt
#include <QMutexLocker>
#include <QSettings>
#include <QStringBuilder>
// QXmpp
#include <QXmppUtils.h>
#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
#include <QXmppSasl2UserAgent.h>
#endif
// Kaidan
#include "AccountDb.h"
#include "EncryptionController.h"
#include "Globals.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "RegistrationManager.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"
#include "Settings.h"
#include "SystemUtils.h"
#include "VCardCache.h"

AccountManager *AccountManager::s_instance = nullptr;

AccountManager *AccountManager::instance()
{
	return s_instance;
}

AccountManager::AccountManager(Settings *settings, VCardCache *cache, QObject *parent)
	: QObject(parent),
	  m_settings(settings),
	  m_port(PORT_AUTODETECT)
{
	Q_ASSERT(!s_instance);
	s_instance = this;

	connect(cache, &VCardCache::vCardChanged, this, [this](const QString &jid) {
		if (m_jid == jid) {
			Q_EMIT displayNameChanged();
		}
	});
}

const Account &AccountManager::account() const
{
	return m_account;
}

void AccountManager::setContactNotificationRule(const QString &jid, Account::ContactNotificationRule rule)
{
	await(
		AccountDb::instance()->updateAccount(
			jid,
			[rule](Account &account) {
				account.contactNotificationRule = rule;
			}
		),
		this,
		[this, rule]() {
			m_account.contactNotificationRule = rule;
			Q_EMIT accountChanged();
		}
	);
}

void AccountManager::setGroupChatNotificationRule(const QString &jid, Account::GroupChatNotificationRule rule)
{
	await(
		AccountDb::instance()->updateAccount(
			jid,
			[rule](Account &account) {
				account.groupChatNotificationRule = rule;
			}
		),
		this,
		[this, rule]() {
			m_account.groupChatNotificationRule = rule;
			Q_EMIT accountChanged();
		}
	);
}

QString AccountManager::jid()
{
	QMutexLocker locker(&m_mutex);
	return m_jid;
}

void AccountManager::setJid(const QString &jid)
{
	QMutexLocker locker(&m_mutex);

	if (m_jid != jid) {
		m_jid = jid;
		m_hasNewCredentials = true;

		// If a server's JID is set during registration, it should not be added as an account.
		// Thus, the JID must contain a local part (user).
		//
		// If the JID is cleared, there is no corresponding account.
		// Thus, the JID must not be empty.
		if (!QXmppUtils::jidToUser(jid).isEmpty()) {
			AccountDb::instance()->addAccount(jid);

			await(AccountDb::instance()->account(jid), this, [this](Account &&account) {
				m_account = std::move(account);
				Q_EMIT accountChanged();
			});

			await(AccountDb::instance()->fetchHttpUploadLimit(jid), this, [](qint64 &&limit) {
				Kaidan::instance()->serverFeaturesCache()->setHttpUploadLimit(limit);
			});
		}

		locker.unlock();
		Q_EMIT jidChanged();
	}
}

void AccountManager::setJidResourcePrefix(const QString &jidResourcePrefix)
{
	m_jidResourcePrefix = jidResourcePrefix;
	m_jidResource = generateJidResourceWithRandomSuffix();
}

QString AccountManager::jidResource() const
{
	return m_jidResourcePrefix.isEmpty() ? generateJidResourceWithRandomSuffix() : m_jidResource;
}

QString AccountManager::password()
{
	QMutexLocker locker(&m_mutex);
	return m_password;
}

void AccountManager::setPassword(const QString &password)
{
	QMutexLocker locker(&m_mutex);

	if (m_password != password) {
		m_password = password;
		m_hasNewCredentials = true;

		locker.unlock();
		Q_EMIT passwordChanged();
	}
}

QString AccountManager::host()
{
	QMutexLocker locker(&m_mutex);
	return m_host;
}

void AccountManager::setHost(const QString &host)
{
	QMutexLocker locker(&m_mutex);

	if (m_host != host) {
		m_host = host;
		m_hasNewConnectionSettings = true;

		locker.unlock();
		Q_EMIT hostChanged();
	}
}

quint16 AccountManager::port()
{
	QMutexLocker locker(&m_mutex);
	return m_port;
}

void AccountManager::setPort(const quint16 port)
{
	QMutexLocker locker(&m_mutex);

	if (m_port != port) {
		m_port = port;
		m_hasNewConnectionSettings = true;

		locker.unlock();
		Q_EMIT portChanged();
	}
}

quint16 AccountManager::portAutodetect() const
{
	return PORT_AUTODETECT;
}

QString AccountManager::displayName()
{
	// no mutex locker required, own attributes are not accessed
	if (const auto vCard = Kaidan::instance()->vCardCache()->vCard(m_jid)) {
		if (const auto nickname = vCard->nickName(); !nickname.isEmpty()) {
			return nickname;
		}
	}

	return QXmppUtils::jidToUser(m_jid);
}

void AccountManager::resetCustomConnectionSettings()
{
	setHost({});
	setPort(PORT_AUTODETECT);
}

bool AccountManager::hasNewCredentials() const
{
	return m_hasNewCredentials;
}

void AccountManager::setHasNewCredentials(bool hasNewCredentials)
{
	m_hasNewCredentials = hasNewCredentials;
}

bool AccountManager::hasEnoughCredentialsForLogin()
{
	return !(jid().isEmpty() || password().isEmpty());
}

bool AccountManager::hasNewConnectionSettings() const
{
	return m_hasNewConnectionSettings;
}

void AccountManager::setHasNewConnectionSettings(bool hasNewConnectionSettings)
{
	m_hasNewConnectionSettings = hasNewConnectionSettings;
}

#if QXMPP_VERSION >= QT_VERSION_CHECK(1, 7, 0)
QXmppSasl2UserAgent AccountManager::userAgent()
{
	auto deviceId = m_settings->userAgentDeviceId();
	if (deviceId.isNull()) {
		deviceId = QUuid::createUuid();
		m_settings->setUserAgentDeviceId(deviceId);
	}
	return QXmppSasl2UserAgent(deviceId, QStringLiteral(APPLICATION_DISPLAY_NAME), SystemUtils::productName());
}
#endif

bool AccountManager::loadConnectionData()
{
	if (!hasEnoughCredentialsForLogin()) {
		// Load the credentials from the settings file.
		setJid(m_settings->authJid());
		setPassword(m_settings->authPassword());

		// Use a default prefix for the JID's resource part if no prefix is already set.
		setJidResourcePrefix(m_settings->authJidResourcePrefix());

		// Load the custom connection setings.
		setHost(m_settings->authHost());
		setPort(m_settings->authPort());

		// This method is only used to load old credentials and connection settings.
		// Thus, "m_hasNewCredentials" and "m_hasNewConnectionSettings" which were set to "true" by
		// setting the credentials and connection settings in this method are reset here.
		m_hasNewCredentials = false;
		m_hasNewConnectionSettings = false;

		// If no credentials could be loaded from the settings file, notify the GUI to ask
		// the user for credentials.
		if (!hasEnoughCredentialsForLogin())
			return false;
	}

	return true;
}

void AccountManager::storeJid()
{
	m_settings->setAuthJid(jid());
}

void AccountManager::storePassword()
{
	m_settings->setAuthPassword(password());
}

void AccountManager::storeCustomConnectionSettings()
{
	if (m_host.isEmpty())
		m_settings->resetAuthHost();
	else
		m_settings->setAuthHost(m_host);

	if (m_settings->isDefaultAuthPort())
		m_settings->resetAuthPort();
	else
		m_settings->setAuthPort(m_port);
}

void AccountManager::storeConnectionData()
{
	storeJid();
	storePassword();
	storeCustomConnectionSettings();
}

void AccountManager::deleteAccountFromClient()
{
	m_deletionStates = DeletionState::ToBeDeletedFromClient;

	// If the client is not yet disconnected, disconnect first and delete the account afterwards.
	// Otherwise, delete the account directly from the client.
	runOnThread(
		Kaidan::instance()->client(),
		[]() {
			return Kaidan::instance()->client()->xmppClient()->isAuthenticated();
		},
		this,
		[this](bool authenticated) {
			if (authenticated) {
				await(EncryptionController::instance()->reset(), this, []() {
					runOnThread(Kaidan::instance()->client(), []() {
						Kaidan::instance()->client()->logOut();
					});
				});
			} else {
				runOnThread(Kaidan::instance()->client(), []() {
					Kaidan::instance()->client()->logIn();
				});
			}
		}
	);
}

void AccountManager::deleteAccountFromClientAndServer()
{
	m_deletionStates = DeletionState::ToBeDeletedFromClient | DeletionState::ToBeDeletedFromServer;

	// If the client is already connected, delete the account directly from the server.
	// Otherwise, connect first and delete the account afterwards.
	runOnThread(
		Kaidan::instance()->client(),
		[]() {
			return Kaidan::instance()->client()->xmppClient()->isAuthenticated();
		},
		this,
		[this](bool authenticated) {
			if (authenticated) {
				runOnThread(Kaidan::instance()->client(), []() {
					Kaidan::instance()->client()->registrationManager()->deleteAccount();
				});
			} else {
				m_deletionStates |= DeletionState::ClientDisconnectedBeforeDeletionFromServer;

				runOnThread(Kaidan::instance()->client(), []() {
					Kaidan::instance()->client()->logIn();
				});
			}
		}
	);
}

void AccountManager::handleAccountDeletedFromServer()
{
	m_deletionStates = DeletionState::DeletedFromServer;
}

void AccountManager::handleAccountDeletionFromServerFailed(const QXmppStanza::Error &error)
{
	Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Your account could not be deleted from the server. Therefore, it was also not removed from this app: %1").arg(error.text()));

	if (m_deletionStates.testFlag(DeletionState::ClientDisconnectedBeforeDeletionFromServer)) {
		m_deletionStates = DeletionState::NotToBeDeleted;

		runOnThread(Kaidan::instance()->client(), []() {
			Kaidan::instance()->client()->logOut();
		});
	} else {
		m_deletionStates = DeletionState::NotToBeDeleted;
	}
}

bool AccountManager::handleConnected()
{
	// If the account could not be deleted because the client was disconnected, delete it now.
	if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient) && m_deletionStates.testFlag(DeletionState::ToBeDeletedFromServer)) {
			runOnThread(Kaidan::instance()->client(), []() {
				Kaidan::instance()->client()->registrationManager()->deleteAccount();
			});

			return true;
		}
	else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
		deleteAccountFromClient();
		return true;
	}

	return false;
}

void AccountManager::handleDisconnected()
{
	// Delete the account from the client if the account was deleted from the server or the client
	// was connected and had to disconnect first.
	if (m_deletionStates.testFlag(DeletionState::DeletedFromServer)) {
		await(EncryptionController::instance()->resetLocally(), this, [this]() {
			removeAccount(m_jid);
		});
	} else if (m_deletionStates.testFlag(DeletionState::ToBeDeletedFromClient)) {
		removeAccount(m_jid);
	}

	m_deletionStates = DeletionState::NotToBeDeleted;
}

QString AccountManager::generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters) const
{
	return m_jidResourcePrefix % QLatin1Char('.') % QXmppUtils::generateStanzaHash(numberOfRandomSuffixCharacters);
}

void AccountManager::removeAccount(const QString &accountJid)
{
	MessageDb::instance()->removeAllMessagesFromAccount(accountJid);
	Q_EMIT RosterModel::instance()->removeItemsRequested(accountJid);
	AccountDb::instance()->removeAccount(accountJid);

	deleteSettings();
	deleteCredentials();
}

void AccountManager::deleteSettings()
{
	m_settings->remove({
		QStringLiteral(KAIDAN_SETTINGS_AUTH_ONLINE),
		QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS),
		QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE),
		QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE),
	});
}

void AccountManager::deleteCredentials()
{
	m_settings->remove({
		QStringLiteral(KAIDAN_SETTINGS_AUTH_JID),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_CREDENTIALS),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT),
		QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY),
	});

	setJid({});
	m_jidResourcePrefix.clear();
	m_jidResource.clear();
	setPassword({});
	resetCustomConnectionSettings();

	Q_EMIT credentialsNeeded();
}
