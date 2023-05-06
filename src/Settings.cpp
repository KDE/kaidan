// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

Encryption::Enum Settings::encryption() const
{
	QMutexLocker locker(&m_mutex);
	return m_settings.value(QStringLiteral(KAIDAN_SETTINGS_ENCRYPTION), Encryption::Omemo2).value<Encryption::Enum>();
}

void Settings::setEncryption(Encryption::Enum encryption)
{
	QMutexLocker locker(&m_mutex);
	m_settings.setValue(QStringLiteral(KAIDAN_SETTINGS_ENCRYPTION), encryption);
	locker.unlock();
	emit encryptionChanged();
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
