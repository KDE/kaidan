// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ServerFeaturesCache.h"

// Qt
#include <QMutexLocker>
// Kaidan
#include "QmlUtils.h"

ServerFeaturesCache::ServerFeaturesCache(QObject *parent)
    : QObject(parent)
{
}

bool ServerFeaturesCache::inBandRegistrationSupported()
{
    QMutexLocker locker(&m_mutex);
    return m_inBandRegistrationSupported;
}

void ServerFeaturesCache::setInBandRegistrationSupported(bool supported)
{
    QMutexLocker locker(&m_mutex);
    if (m_inBandRegistrationSupported != supported) {
        m_inBandRegistrationSupported = supported;
        locker.unlock();
        Q_EMIT inBandRegistrationSupportedChanged();
    }
}

bool ServerFeaturesCache::httpUploadSupported()
{
    QMutexLocker locker(&m_mutex);
    return m_httpUploadSupported;
}

void ServerFeaturesCache::setHttpUploadSupported(bool supported)
{
    QMutexLocker locker(&m_mutex);
    if (m_httpUploadSupported != supported) {
        m_httpUploadSupported = supported;
        locker.unlock();
        Q_EMIT httpUploadSupportedChanged();
    }
}

qint64 ServerFeaturesCache::httpUploadLimit()
{
    QMutexLocker locker(&m_mutex);
    return m_httpUploadLimit;
}

QString ServerFeaturesCache::httpUploadLimitString()
{
    return QmlUtils::formattedDataSize(httpUploadLimit());
}

void ServerFeaturesCache::setHttpUploadLimit(qint64 size)
{
    QMutexLocker locker(&m_mutex);
    if (m_httpUploadLimit != size) {
        m_httpUploadLimit = size;
        locker.unlock();
        Q_EMIT httpUploadLimitChanged();
    }
}

#include "moc_ServerFeaturesCache.cpp"
