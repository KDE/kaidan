// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class OmemoWatcher : public QObject
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

	Q_PROPERTY(QList<QString> distrustedOmemoDevices READ distrustedOmemoDevices NOTIFY distrustedOmemoDevicesChanged)
	Q_PROPERTY(QList<QString> usableOmemoDevices READ usableOmemoDevices NOTIFY usableOmemoDevicesChanged)
	Q_PROPERTY(QList<QString> authenticatableOmemoDevices READ authenticatableOmemoDevices NOTIFY authenticatableOmemoDevicesChanged)

public:
	OmemoWatcher() = default;

	QString jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	QList<QString> distrustedOmemoDevices() const;
	Q_SIGNAL void distrustedOmemoDevicesChanged();

	QList<QString> usableOmemoDevices() const;
	Q_SIGNAL void usableOmemoDevicesChanged();

	QList<QString> authenticatableOmemoDevices() const;
	Q_SIGNAL void authenticatableOmemoDevicesChanged();

private:
	void handleDistrustedOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);
	void handleUsableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);
	void handleAuthenticatableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels);

	QString m_jid;

	QList<QString> m_distrustedOmemoDevices;
	QList<QString> m_usableOmemoDevices;
	QList<QString> m_authenticatableOmemoDevices;
};

