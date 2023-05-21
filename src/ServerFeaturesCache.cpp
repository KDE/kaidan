// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ServerFeaturesCache.h"
// Qt
#include <QMutexLocker>

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
		emit inBandRegistrationSupportedChanged();
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
		emit httpUploadSupportedChanged();
	}
}
