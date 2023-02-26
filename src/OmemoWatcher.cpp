/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OmemoCache.h"
#include "OmemoWatcher.h"

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
