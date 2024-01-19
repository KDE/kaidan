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

void OmemoCache::setAuthenticatableKeys(const QString &jid, const QList<QString> &authenticatableKeys)
{
	QMutexLocker locker(&m_mutex);
	auto devicesChanged = false;
	auto &authenticatableDevices = m_authenticatableDevices[jid];

	for (const auto &authenticatableKey : authenticatableKeys) {
		auto keyFound = false;

		for (const auto &authenticatableDevice : authenticatableDevices) {
			if (authenticatableDevice.keyId == authenticatableKey || m_ownDevice.keyId == authenticatableKey) {
				keyFound = true;
				break;
			}
		}

		// Add a key for whom no device is stored as a new device without a label.
		if (!keyFound) {
			authenticatableDevices.append({ {}, authenticatableKey });

			if (!devicesChanged) {
				devicesChanged = true;
			}
		}
	}

	if (devicesChanged) {
		locker.unlock();
		Q_EMIT authenticatedDevicesUpdated(jid, authenticatableDevices);
	}
}

void OmemoCache::setAuthenticatedKeys(const QString &jid, const QList<QString> &authenticatedKeys)
{
	QMutexLocker locker(&m_mutex);
	auto devicesChanged = false;
	auto &authenticatedDevices = m_authenticatedDevices[jid];

	for (const auto &authenticatedKey : authenticatedKeys) {
		auto keyFound = false;

		for (const auto &authenticatedDevice : authenticatedDevices) {
			if (authenticatedDevice.keyId == authenticatedKey || m_ownDevice.keyId == authenticatedKey) {
				keyFound = true;
				break;
			}
		}

		// Add a key for whom no device is stored as a new device without a label.
		if (!keyFound) {
			authenticatedDevices.append({ {}, authenticatedKey });

			if (!devicesChanged) {
				devicesChanged = true;
			}
		}
	}

	if (devicesChanged) {
		locker.unlock();
		Q_EMIT authenticatedDevicesUpdated(jid, authenticatedDevices);
	}
}

void OmemoCache::setOwnDevice(const OmemoManager::Device &ownDevice)
{
	QMutexLocker locker(&m_mutex);

	if (ownDevice != m_ownDevice) {
		m_ownDevice = ownDevice;
		locker.unlock();
		Q_EMIT ownDeviceUpdated(ownDevice);
	}
}

OmemoManager::Device OmemoCache::ownDevice()
{
	QMutexLocker locker(&m_mutex);
	return m_ownDevice;
}

void OmemoCache::setDistrustedDevices(const QString &jid, const QList<OmemoManager::Device> &distrustedDevices)
{
	QMutexLocker locker(&m_mutex);

	if (distrustedDevices != m_distrustedDevices.value(jid)) {
		m_distrustedDevices.insert(jid, distrustedDevices);
		locker.unlock();
		Q_EMIT distrustedDevicesUpdated(jid, distrustedDevices);
	}
}

QList<OmemoManager::Device> OmemoCache::distrustedDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_distrustedDevices.value(jid);
}

void OmemoCache::setUsableDevices(const QString &jid, const QList<OmemoManager::Device> &usableDevices)
{
	QMutexLocker locker(&m_mutex);

	if (usableDevices != m_usableDevices.value(jid)) {
		m_usableDevices.insert(jid, usableDevices);
		locker.unlock();
		Q_EMIT usableDevicesUpdated(jid, usableDevices);
	}
}

QList<OmemoManager::Device> OmemoCache::usableDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_usableDevices.value(jid);
}

void OmemoCache::setAuthenticatableDevices(const QString &jid, const QList<OmemoManager::Device> &authenticatableDevices)
{
	QMutexLocker locker(&m_mutex);

	if (authenticatableDevices != m_authenticatableDevices.value(jid)) {
		m_authenticatableDevices.insert(jid, authenticatableDevices);
		locker.unlock();
		Q_EMIT authenticatableDevicesUpdated(jid, authenticatableDevices);
	}
}

QList<OmemoManager::Device> OmemoCache::authenticatableDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_authenticatableDevices.value(jid);
}

void OmemoCache::setAuthenticatedDevices(const QString &jid, const QList<OmemoManager::Device> &authenticatedDevices)
{
	QMutexLocker locker(&m_mutex);

	if (authenticatedDevices != m_authenticatedDevices.value(jid)) {
		m_authenticatedDevices.insert(jid, authenticatedDevices);
		locker.unlock();
		Q_EMIT authenticatedDevicesUpdated(jid, authenticatedDevices);
	}
}

QList<OmemoManager::Device> OmemoCache::authenticatedDevices(const QString &jid)
{
	QMutexLocker locker(&m_mutex);
	return m_authenticatedDevices.value(jid);
}
