/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "AccountManager.h"
// Qt
#include <QMutexLocker>
#include <QSettings>
#include <QStringBuilder>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Globals.h"
#include "Kaidan.h"
#include "MessageModel.h"
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

	connect(cache, &VCardCache::vCardChanged, this, [=](const QString &jid) {
		if (m_jid == jid) {
			emit displayNameChanged();
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

		locker.unlock();
		emit jidChanged();
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
		emit passwordChanged();
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
		m_hasNewCredentials = true;

		locker.unlock();
		emit hostChanged();
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
		m_hasNewCredentials = true;

		locker.unlock();
		emit portChanged();
	}
}

quint16 AccountManager::nonCustomPort() const
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

bool AccountManager::loadCredentials()
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

		// This method is only used to load old credentials. Therefore,
		// "m_hasNewCredentials" which was set to "true" by setting the credentials in this
		// method is reset here.
		m_hasNewCredentials = false;

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

void AccountManager::storeCredentials()
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

	emit credentialsNeeded();
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

	emit MessageModel::instance()->removeMessagesRequested(accountJid);
	emit RosterModel::instance()->removeItemsRequested(accountJid);
}

QString AccountManager::generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters) const
{
	return m_jidResourcePrefix % "." % QXmppUtils::generateStanzaHash(numberOfRandomSuffixCharacters);
}
