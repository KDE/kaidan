// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

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

	/**
	 * Emitted when there are OMEMO devices with distrusted keys.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void distrustedOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);

	/**
	 * Emitted when there are OMEMO devices usable for end-to-end encryption.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void usableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);

	/**
	 * Emitted when there are OMEMO devices with keys that can be authenticated.
	 *
	 * @param jid JID of the device's owner
	 * @param deviceLabels human-readable strings used to identify the devices
	 */
	Q_SIGNAL void authenticatableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);

private:
	static OmemoCache *s_instance;
};
