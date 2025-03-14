// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Settings.h"

// Qt
#include <QMutexLocker>
// Kaidan
#include "Globals.h"
#include "Kaidan.h"

Settings::Settings(QObject *parent)
    : QObject(parent)
    , m_settings(QStringLiteral(APPLICATION_NAME), configFileBaseName())
{
}

QSettings &Settings::raw()
{
    return m_settings;
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

void Settings::remove(const QStringList &keys)
{
    QMutexLocker locker(&m_mutex);
    for (const QString &key : keys)
        m_settings.remove(key);
}

#include "moc_Settings.cpp"
