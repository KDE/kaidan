// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountManager.h"
// Qt
#include <QMutexLocker>
#include <QSettings>
#include <QStringBuilder>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
#include "Globals.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "RosterModel.h"
#include "Settings.h"
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

		AccountDb::instance()->addAccount(jid);

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

void AccountManager::deleteCredentials()
{
	m_settings->remove({
		KAIDAN_SETTINGS_AUTH_JID,
		KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX,
		KAIDAN_SETTINGS_AUTH_PASSWD,
		KAIDAN_SETTINGS_AUTH_HOST,
		KAIDAN_SETTINGS_AUTH_PORT,
		KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY
	});

	setJid({});
	m_jidResourcePrefix.clear();
	m_jidResource.clear();
	setPassword({});
	resetCustomConnectionSettings();

	Q_EMIT credentialsNeeded();
}

void AccountManager::deleteSettings()
{
	m_settings->remove({
		KAIDAN_SETTINGS_AUTH_ONLINE,
		KAIDAN_SETTINGS_NOTIFICATIONS_MUTED,
		KAIDAN_SETTINGS_FAVORITE_EMOJIS,
		KAIDAN_SETTINGS_HELP_VISIBILITY_QR_CODE_PAGE,
	});
}

void AccountManager::removeAccount(const QString &accountJid)
{
	deleteSettings();
	deleteCredentials();

	MessageDb::instance()->removeAllMessagesFromAccount(accountJid);
	Q_EMIT RosterModel::instance()->removeItemsRequested(accountJid);
}

QString AccountManager::generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters) const
{
	return m_jidResourcePrefix % "." % QXmppUtils::generateStanzaHash(numberOfRandomSuffixCharacters);
}
