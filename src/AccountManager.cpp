/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2020 Kaidan developers and contributors
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
#include <QMutex>
#include <QMutexLocker>
#include <QSettings>
#include <QStringBuilder>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Globals.h"

AccountManager::AccountManager(QSettings *settings, QObject* parent)
	: QObject(parent), m_settings(settings)
{
}

QString AccountManager::jid()
{
	QMutexLocker locker(&m_mutex);
	return m_jid;
}

void AccountManager::setJid(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	m_jid = jid;
	m_hasNewCredentials = true;
	locker.unlock();
	emit jidChanged();
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
	m_password = password;
	m_hasNewCredentials = true;
	locker.unlock();
	emit passwordChanged();
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
		setJid(m_settings->value(KAIDAN_SETTINGS_AUTH_JID).toString());
		setPassword(QByteArray::fromBase64(m_settings->value(KAIDAN_SETTINGS_AUTH_PASSWD).toString().toUtf8()));

		// Use a default prefix for the JID's resource part if no prefix is already set.
		setJidResourcePrefix(m_settings->value(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX, KAIDAN_JID_RESOURCE_DEFAULT_PREFIX).toString());

		// This method is only used to load old credentials. Therefore,
		// "m_hasNewCredentials" which was set to "true" by setting the credentials in this
		// method is reset here.
		m_hasNewCredentials = false;

		// If no credentials could be loaded from the settings file, notify the GUI to ask
		// the user for credentials.
		if (!hasEnoughCredentialsForLogin()) {
			emit newCredentialsNeeded();
			return false;
		}
	}

	return true;
}

void AccountManager::storeJidInSettingsFile()
{
	m_settings->setValue(KAIDAN_SETTINGS_AUTH_JID, jid());
}

void AccountManager::storePasswordInSettingsFile()
{
	m_settings->setValue(KAIDAN_SETTINGS_AUTH_PASSWD, QString::fromUtf8(password().toUtf8().toBase64()));
}

void AccountManager::deleteCredentials()
{
	m_settings->remove(KAIDAN_SETTINGS_AUTH_JID);
	m_settings->remove(KAIDAN_SETTINGS_AUTH_PASSWD);

	setJid({});
	m_jidResourcePrefix.clear();
	m_jidResource.clear();
	setPassword({});

	emit newCredentialsNeeded();
}

QString AccountManager::generateJidResourceWithRandomSuffix(unsigned int numberOfRandomSuffixCharacters) const
{
	return m_jidResourcePrefix % "." % QXmppUtils::generateStanzaHash(numberOfRandomSuffixCharacters);
}
