// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class OmemoWatcher : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

	Q_PROPERTY(QList<QString> distrustedDevices READ distrustedDevices NOTIFY distrustedDevicesChanged)
	Q_PROPERTY(QList<QString> usableDevices READ usableDevices NOTIFY usableDevicesChanged)
	Q_PROPERTY(QList<QString> authenticatableDevices READ authenticatableDevices NOTIFY authenticatableDevicesChanged)

public:
	OmemoWatcher() = default;

	QString jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	QList<QString> distrustedDevices() const;
	Q_SIGNAL void distrustedDevicesChanged();

	QList<QString> usableDevices() const;
	Q_SIGNAL void usableDevicesChanged();

	QList<QString> authenticatableDevices() const;
	Q_SIGNAL void authenticatableDevicesChanged();

private:
	void handleDistrustedDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);
	void handleUsableDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);
	void handleAuthenticatableDevicesUpdated(const QString &jid, const QList<QString> &deviceLabels);

	QString m_jid;

	QList<QString> m_distrustedDevices;
	QList<QString> m_usableDevices;
	QList<QString> m_authenticatableDevices;
};

