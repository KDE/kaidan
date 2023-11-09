// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoCache.h"

#include <QMutexLocker>

OmemoCache *OmemoCache::s_instance = nullptr;

OmemoCache::OmemoCache(QObject *parent)
	: QObject(parent)
{
	Q_ASSERT(!s_instance);
	s_instance = this;
}

OmemoCache::~OmemoCache()
{
	s_instance = nullptr;
}

void OmemoCache::setDistrustedDevices(const QString &jid, const QList<QString> &distrustedDevices)
{
	QMutexLocker locker(&m_mutex);

	if (distrustedDevices != m_distrustedDevices.value(jid)) {
		m_distrustedDevices.insert(jid, distrustedDevices);
		locker.unlock();
		Q_EMIT distrustedDevicesUpdated(jid, distrustedDevices);
	}
}

QList<QString> OmemoCache::distrustedDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_distrustedDevices.value(jid);
}

void OmemoCache::setUsableDevices(const QString &jid, const QList<QString> &usableDevices)
{
	QMutexLocker locker(&m_mutex);

	if (usableDevices != m_usableDevices.value(jid)) {
		m_usableDevices.insert(jid, usableDevices);
		locker.unlock();
		Q_EMIT usableDevicesUpdated(jid, usableDevices);
	}
}

QList<QString> OmemoCache::usableDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_usableDevices.value(jid);
}

void OmemoCache::setAuthenticatableDevices(const QString &jid, const QList<QString> &authenticatableDevices)
{
	QMutexLocker locker(&m_mutex);

	if (authenticatableDevices != m_authenticatableDevices.value(jid)) {
		m_authenticatableDevices.insert(jid, authenticatableDevices);
		locker.unlock();
		Q_EMIT authenticatableDevicesUpdated(jid, authenticatableDevices);
	}
}

QList<QString> OmemoCache::authenticatableDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_authenticatableDevices.value(jid);
}
