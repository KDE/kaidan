/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
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

#include "Settings.h"

#include <QMutexLocker>

#include "Globals.h"

Settings::Settings(QObject *parent)
	: QObject(parent), m_settings(QStringLiteral(APPLICATION_NAME), configFileBaseName())
{

}

QSettings &Settings::raw()
{
	return m_settings;
}

bool Settings::authOnline() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_ONLINE), true).toBool();
}

void Settings::setAuthOnline(bool online)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_ONLINE), online);
	locker.unlock();
	emit authOnlineChanged();
}

QString Settings::authJid() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID)).toString();
}

void Settings::setAuthJid(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID), jid);
	locker.unlock();
	emit authJidChanged();
}

QString Settings::authJidResourcePrefix() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), KAIDAN_JID_RESOURCE_DEFAULT_PREFIX).toString();
}

void Settings::setAuthJidResourcePrefix(const QString &prefix)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), prefix);
	locker.unlock();
	emit authJidResourcePrefixChanged();
}

QString Settings::authPassword() const
{
	QMutexLocker locker(&m_mutex);
	return QByteArray::fromBase64(m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD)).toString().toUtf8());
}

void Settings::setAuthPassword(const QString &password)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD), QString::fromUtf8(password.toUtf8().toBase64()));
	locker.unlock();
	emit authPasswordChanged();
}

QString Settings::authHost() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST)).toString();
}

void Settings::setAuthHost(const QString &host)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST), host);
	locker.unlock();
	emit authHostChanged();
}

void Settings::resetAuthHost()
{
	QMutexLocker locker(&m_mutex);
	m_settings.remove(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST));
	locker.unlock();
	emit authHostChanged();
}

quint16 Settings::authPort() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT), PORT_AUTODETECT).value<quint16>();
}

void Settings::setAuthPort(quint16 port)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT), port);
	locker.unlock();
	emit authPortChanged();
}

void Settings::resetAuthPort()
{
	QMutexLocker locker(&m_mutex);
	m_settings.remove(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT));
	locker.unlock();
	emit authPortChanged();
}

bool Settings::isDefaultAuthPort() const
{
	return authPort() == PORT_AUTODETECT;
}

Kaidan::PasswordVisibility Settings::authPasswordVisibility() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), Kaidan::PasswordVisible).value<Kaidan::PasswordVisibility>();
}

void Settings::setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), visibility);
	locker.unlock();
	emit authPasswordVisibilityChanged();
}

bool Settings::qrCodePageExplanationVisible() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_HELP_VISIBILITY_QR_CODE_PAGE), true).toBool();
}

void Settings::setQrCodePageExplanationVisible(bool isVisible)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_HELP_VISIBILITY_QR_CODE_PAGE), isVisible);
	locker.unlock();
	emit qrCodePageExplanationVisibleChanged();
}

bool Settings::notificationsMuted(const QString &bareJid) const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_NOTIFICATIONS_MUTED) + bareJid, false).toBool();
}

void Settings::setNotificationsMuted(const QString &bareJid, bool muted)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_NOTIFICATIONS_MUTED) + bareJid, muted);
	locker.unlock();
	emit notificationsMutedChanged(bareJid);
}

QStringList Settings::favoriteEmojis() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS)).toStringList();
}

void Settings::setFavoriteEmojis(const QStringList &emoji)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS), emoji);
	locker.unlock();
	emit favoriteEmojisChanged();
}

QPoint Settings::windowPosition() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_WINDOW_POSITION)).toPoint();
}

void Settings::setWindowPosition(const QPoint &windowPosition)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_WINDOW_POSITION), windowPosition);
	locker.unlock();
	emit windowPositionChanged();
}

QSize Settings::windowSize() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_WINDOW_SIZE)).toSize();
}

void Settings::setWindowSize(const QSize &windowSize)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_WINDOW_SIZE), windowSize);
	locker.unlock();
	emit windowSizeChanged();
}

void Settings::remove(const QStringList &keys)
{
	QMutexLocker locker(&m_mutex);
	for (const QString &key : keys)
		m_settings.remove(key);
}
