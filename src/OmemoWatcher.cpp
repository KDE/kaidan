// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoWatcher.h"

#include "OmemoCache.h"

QString OmemoWatcher::jid() const
{
	return m_jid;
}

void OmemoWatcher::setJid(const QString &jid)
{
	m_jid = jid;

	connect(OmemoCache::instance(), &OmemoCache::distrustedOmemoDevicesRetrieved, this, &OmemoWatcher::handleDistrustedOmemoDevicesRetrieved);
	connect(OmemoCache::instance(), &OmemoCache::usableOmemoDevicesRetrieved, this, &OmemoWatcher::handleUsableOmemoDevicesRetrieved);
	connect(OmemoCache::instance(), &OmemoCache::authenticatableOmemoDevicesRetrieved, this, &OmemoWatcher::handleAuthenticatableOmemoDevicesRetrieved);

	Q_EMIT jidChanged();
}

QList<QString> OmemoWatcher::distrustedOmemoDevices() const
{
	return m_distrustedOmemoDevices;
}

QList<QString> OmemoWatcher::usableOmemoDevices() const
{
	return m_usableOmemoDevices;
}

QList<QString> OmemoWatcher::authenticatableOmemoDevices() const
{
	return m_authenticatableOmemoDevices;
}

void OmemoWatcher::handleDistrustedOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels)
{
	if (jid == m_jid) {
		m_distrustedOmemoDevices = deviceLabels;
		emit distrustedOmemoDevicesChanged();
	}
}

void OmemoWatcher::handleUsableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels)
{
	if (jid == m_jid) {
		m_usableOmemoDevices = deviceLabels;
		emit usableOmemoDevicesChanged();
	}
}

void OmemoWatcher::handleAuthenticatableOmemoDevicesRetrieved(const QString &jid, const QList<QString> &deviceLabels)
{
	if (jid == m_jid) {
		m_authenticatableOmemoDevices = deviceLabels;
		emit authenticatableOmemoDevicesChanged();
	}
}
