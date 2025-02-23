// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.h"

#include <QMutexLocker>
#include <QUuid>

#include <QXmppCredentials.h>

#include "Globals.h"

Q_DECLARE_METATYPE(QXmppConfiguration::StreamSecurityMode)

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral(APPLICATION_NAME), configFileBaseName())
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
    return value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), QStringLiteral(KAIDAN_JID_RESOURCE_DEFAULT_PREFIX));
}

void Settings::setAuthJidResourcePrefix(const QString &prefix)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_JID_RESOURCE_PREFIX), prefix, &Settings::authJidResourcePrefixChanged);
}

QString Settings::authPassword() const
{
    return QString::fromUtf8(QByteArray::fromBase64(value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD)).toUtf8()));
}

void Settings::setAuthPassword(const QString &password)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD), QString::fromUtf8(password.toUtf8().toBase64()), &Settings::authPasswordChanged);
}

QXmppCredentials Settings::authCredentials() const
{
    auto xml = value<QString>(QStringLiteral(KAIDAN_SETTINGS_AUTH_CREDENTIALS));

    QXmlStreamReader r(xml);
    r.readNextStartElement();
    return QXmppCredentials::fromXml(r).value_or(QXmppCredentials());
}

void Settings::setAuthCredentials(const QXmppCredentials &credentials)
{
    QString xml;
    QXmlStreamWriter writer(&xml);
    credentials.toXml(writer);

    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_CREDENTIALS), xml);
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

bool Settings::authTlsErrorsIgnored() const
{
    return value<bool>(QStringLiteral(KAIDAN_SETTINGS_AUTH_TLS_ERRORS_IGNORED), false);
}

void Settings::setAuthTlsErrorsIgnored(bool enabled)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_TLS_ERRORS_IGNORED), enabled, &Settings::authIgnoreTlsErrosChanged);
}

QXmppConfiguration::StreamSecurityMode Settings::authTlsRequirement() const
{
    return value<QXmppConfiguration::StreamSecurityMode>(QStringLiteral(KAIDAN_SETTINGS_AUTH_TLS_REQUIREMENT), QXmppConfiguration::TLSRequired);
}

void Settings::setAuthTlsRequirement(QXmppConfiguration::StreamSecurityMode mode)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_TLS_REQUIREMENT), mode, &Settings::authTlsRequirementChanged);
}

Kaidan::PasswordVisibility Settings::authPasswordVisibility() const
{
    return value<Kaidan::PasswordVisibility>(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), Kaidan::PasswordVisible);
}

void Settings::setAuthPasswordVisibility(Kaidan::PasswordVisibility visibility)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_PASSWD_VISIBILITY), visibility, &Settings::authPasswordVisibilityChanged);
}

QUuid Settings::userAgentDeviceId() const
{
    return value<QUuid>(QString::fromLatin1(KAIDAN_SETTINGS_AUTH_DEVICE_ID));
}

void Settings::setUserAgentDeviceId(QUuid deviceId)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTH_DEVICE_ID), deviceId);
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
    setValue(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_CONTACT_ADDITION_QR_CODE_PAGE),
             visible,
             &Settings::contactAdditionQrCodePageExplanationVisibleChanged);
}

bool Settings::keyAuthenticationPageExplanationVisible() const
{
    return value<bool>(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE), true);
}

void Settings::setKeyAuthenticationPageExplanationVisible(bool visible)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_EXPLANATION_VISIBILITY_KEY_AUTHENTICATION_PAGE),
             visible,
             &Settings::keyAuthenticationPageExplanationVisibleChanged);
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

Account::AutomaticMediaDownloadsRule Settings::automaticMediaDownloadsRule() const
{
    return value<Account::AutomaticMediaDownloadsRule>(QStringLiteral(KAIDAN_SETTINGS_AUTOMATIC_MEDIA_DOWNLOADS_RULE),
                                                       Account::AutomaticMediaDownloadsRule::Default);
}

void Settings::setAutomaticMediaDownloadsRule(Account::AutomaticMediaDownloadsRule rule)
{
    setValue(QStringLiteral(KAIDAN_SETTINGS_AUTOMATIC_MEDIA_DOWNLOADS_RULE), rule, &Settings::automaticMediaDownloadsRuleChanged);
}

void Settings::remove(const QStringList &keys)
{
    QMutexLocker locker(&m_mutex);
    for (const QString &key : keys)
        m_settings.remove(key);
}

#include "moc_Settings.cpp"
