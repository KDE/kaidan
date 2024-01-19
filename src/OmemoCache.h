// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QMap>
#include <QMutex>
#include <QObject>
// Kaidan
#include "OmemoManager.h"

class OmemoCache : public QObject
{
	Q_OBJECT

public:
	OmemoCache(QObject *parent = nullptr);
	~OmemoCache();

	static OmemoCache *instance()
	{
		return s_instance;
	}

	void setAuthenticatableKeys(const QString &jid, const QList<QString> &authenticatableKeys);
	void setAuthenticatedKeys(const QString &jid, const QList<QString> &authenticatedKeys);

	void setOwnDevice(const OmemoManager::Device &ownDevice);
	OmemoManager::Device ownDevice();
	Q_SIGNAL void ownDeviceUpdated(const OmemoManager::Device &ownDevice);

	void setDistrustedDevices(const QString &jid, const QList<OmemoManager::Device> &distrustedDevices);
	QList<OmemoManager::Device> distrustedDevices(const QString &jid);
	Q_SIGNAL void distrustedDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &distrustedDevices);

	void setUsableDevices(const QString &jid, const QList<OmemoManager::Device> &usableDevices);
	QList<OmemoManager::Device> usableDevices(const QString &jid);
	Q_SIGNAL void usableDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &usableDevices);

	void setAuthenticatableDevices(const QString &jid, const QList<OmemoManager::Device> &authenticatableDevices);
	QList<OmemoManager::Device> authenticatableDevices(const QString &jid);
	Q_SIGNAL void authenticatableDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &authenticatableDevices);

	void setAuthenticatedDevices(const QString &jid, const QList<OmemoManager::Device> &authenticatedDevices);
	QList<OmemoManager::Device> authenticatedDevices(const QString &jid);
	Q_SIGNAL void authenticatedDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &authenticatedDevices);

private:
	QMutex m_mutex;

	OmemoManager::Device m_ownDevice;

	QMap<QString, QList<OmemoManager::Device>> m_distrustedDevices;
	QMap<QString, QList<OmemoManager::Device>> m_usableDevices;
	QMap<QString, QList<OmemoManager::Device>> m_authenticatableDevices;
	QMap<QString, QList<OmemoManager::Device>> m_authenticatedDevices;

	static OmemoCache *s_instance;
};
