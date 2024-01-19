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
	return value<bool>(QStringLiteral(KAIDAN_SETTINGS_AUTH_ONLINE), true);
}

void Settings::setAuthOnline(bool online)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_ONLINE), online, &Settings::authOnlineChanged);
}

QString Settings::authJid() const
{
	return value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID));
}

void Settings::setAuthJid(const QString &jid)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID), jid, &Settings::authJidChanged);
}

QString Settings::authJidResourcePrefix() const
{
	return value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), KAIDAN_JID_RESOURCE_DEFAULT_PREFIX);
}

void Settings::setAuthJidResourcePrefix(const QString &prefix)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), prefix, &Settings::authJidResourcePrefixChanged);
}

QString Settings::authPassword() const
{
	return QByteArray::fromBase64(value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD)).toUtf8());
}

void Settings::setAuthPassword(const QString &password)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD), QString::fromUtf8(password.toUtf8().toBase64()), &Settings::authPasswordChanged);
}

QString Settings::authHost() const
{
	return value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST));
}

void Settings::setAuthHost(const QString &host)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST), host, &Settings::authHostChanged);
}

void Settings::resetAuthHost()
{
	remove(QStringLiteral(KAIDAN_SETTINGS_AUTH_HOST), &Settings::authHostChanged);
}

quint16 Settings::authPort() const
{
	return value<quint16>(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT), PORT_AUTODETECT);
}

void Settings::setAuthPort(quint16 port)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT), port, &Settings::authPortChanged);
}

void Settings::resetAuthPort()
{
	remove(QStringLiteral(KAIDAN_SETTINGS_AUTH_PORT), &Settings::authPortChanged);
}

bool Settings::isDefaultAuthPort() const
{
	return authPort() == PORT_AUTODETECT;
}

Kaidan::PasswordVisibility Settings::authPasswordVisibility() const
{
	return value<Kaidan::PasswordVisibility>(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), Kaidan::PasswordVisible);
}

void Settings::setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), visibility, &Settings::authPasswordVisibilityChanged);
}

Encryption::Enum Settings::encryption() const
{
	return value<Encryption::Enum>(QStringLiteral(KAIDAN_SETTINGS_ENCRYPTION), Encryption::Omemo2);
}

void Settings::setEncryption(Encryption::Enum encryption)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_ENCRYPTION), encryption, &Settings::encryptionChanged);
}

bool Settings::contactAdditionQrCodePageExplanationVisible() const
{
	return value<bool>(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE), true);
}

void Settings::setContactAdditionQrCodePageExplanationVisible(bool visible)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE), visible, &Settings::contactAdditionQrCodePageExplanationVisibleChanged);
}

bool Settings::keyAuthenticationPageExplanationVisible() const
{
	return value<bool>(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE), true);
}

void Settings::setKeyAuthenticationPageExplanationVisible(bool visible)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE), visible, &Settings::keyAuthenticationPageExplanationVisibleChanged);
}

QStringList Settings::favoriteEmojis() const
{
	return value<QStringList>(QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS));
}

void Settings::setFavoriteEmojis(const QStringList &emoji)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_FAVORITE_EMOJIS), emoji, &Settings::favoriteEmojisChanged);
}

QPoint Settings::windowPosition() const
{
	return value<QPoint>(QStringLiteral(KAIDAN_SETTINGS_WINDOW_POSITION));
}

void Settings::setWindowPosition(const QPoint &windowPosition)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_WINDOW_POSITION), windowPosition, &Settings::windowPositionChanged);
}

QSize Settings::windowSize() const
{
	return value<QSize>(QStringLiteral(KAIDAN_SETTINGS_WINDOW_SIZE));
}

void Settings::setWindowSize(const QSize &windowSize)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_WINDOW_SIZE), windowSize, &Settings::windowSizeChanged);
}

AccountManager::AutomaticMediaDownloadsRule Settings::automaticMediaDownloadsRule() const
{
	return value<AccountManager::AutomaticMediaDownloadsRule>(QStringLiteral(KAIDAN_SETTINGS_AUTOMATIC_MEDIA_DOWNLOADS_RULE), AccountManager::AutomaticMediaDownloadsRule::Default);
}

void Settings::setAutomaticMediaDownloadsRule(AccountManager::AutomaticMediaDownloadsRule rule)
{
	setValue(QStringLiteral(KAIDAN_SETTINGS_AUTOMATIC_MEDIA_DOWNLOADS_RULE), rule, &Settings::automaticMediaDownloadsRuleChanged);
}

void Settings::remove(const QStringList &keys)
{
	QMutexLocker locker(&m_mutex);
	for (const QString &key : keys)
		m_settings.remove(key);
}
