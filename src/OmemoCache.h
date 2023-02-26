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

#pragma once

#include <QObject>

/**
 * @class OmemoCache A cache for presence holders for certain JIDs
 */
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
