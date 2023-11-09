// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QMap>
#include <QMutex>
#include <QObject>

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

	void setDistrustedDevices(const QString &jid, const QList<QString> &distrustedDevices);
	QList<QString> distrustedDevices(const QString &jid);
	/**
	 * Emitted when there are OMEMO devices with distrusted keys.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void distrustedDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);

	void setUsableDevices(const QString &jid, const QList<QString> &usableDevices);
	QList<QString> usableDevices(const QString &jid);
	/**
	 * Emitted when there are OMEMO devices usable for end-to-end encryption.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void usableDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);

	void setAuthenticatableDevices(const QString &jid, const QList<QString> &authenticatableDevices);
	QList<QString> authenticatableDevices(const QString &jid);
	/**
	 * Emitted when there are OMEMO devices with keys that can be authenticated.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void authenticatableDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);

private:
	QMutex m_mutex;
	QMap<QString, QList<QString>> m_distrustedDevices;
	QMap<QString, QList<QString>> m_usableDevices;
	QMap<QString, QList<QString>> m_authenticatableDevices;

	static OmemoCache *s_instance;
};
