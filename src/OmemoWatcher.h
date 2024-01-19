// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// Kaidan
#include "OmemoManager.h"

class OmemoWatcher : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

	Q_PROPERTY(QList<OmemoManager::Device> distrustedDevices READ distrustedDevices NOTIFY distrustedDevicesChanged)
	Q_PROPERTY(QList<OmemoManager::Device> usableDevices READ usableDevices NOTIFY usableDevicesChanged)
	Q_PROPERTY(QList<OmemoManager::Device> authenticatableDevices READ authenticatableDevices NOTIFY authenticatableDevicesChanged)

public:
	OmemoWatcher() = default;

	QString jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	QList<OmemoManager::Device> distrustedDevices() const;
	Q_SIGNAL void distrustedDevicesChanged();

	QList<OmemoManager::Device> usableDevices() const;
	Q_SIGNAL void usableDevicesChanged();

	QList<OmemoManager::Device> authenticatableDevices() const;
	Q_SIGNAL void authenticatableDevicesChanged();

private:
	void handleDistrustedDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &distrustedDevices);
	void handleUsableDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &usableDevices);
	void handleAuthenticatableDevicesUpdated(const QString &jid, const QList<OmemoManager::Device> &authenticatableDevices);

	QString m_jid;

	QList<OmemoManager::Device> m_distrustedDevices;
	QList<OmemoManager::Device> m_usableDevices;
	QList<OmemoManager::Device> m_authenticatableDevices;
};

