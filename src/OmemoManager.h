/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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

#include <QFuture>
#include <QObject>

#include "QXmppTrustLevel.h"

class Database;
class OmemoDb;
class QXmppClient;
class QXmppOmemoManager;

class OmemoManager : public QObject
{
	Q_OBJECT

public:
	OmemoManager(QXmppClient *client, Database *database, QObject *parent = nullptr);
	~OmemoManager();

	/**
	 * Sets the JID of the current account used to store the corresponding data for a specific
	 * account.
	 *
	 * @param accountJid bare JID of the current account
	 */
	void setAccountJid(const QString &accountJid);

	QFuture<void> load();
	QFuture<void> setUp();

	QFuture<void> retrieveKeys(const QList<QString> &jids);

	QFuture<void> requestDeviceLists(const QList<QString> &jids);
	QFuture<void> subscribeToDeviceLists(const QList<QString> &jids);
	QFuture<void> unsubscribeFromDeviceLists();
	void resetOwnDevice();

	QFuture<void> initializeChat(const QString &accountJid, const QString &chatJid);
	void removeContactDevices(const QString &jid);

	Q_SIGNAL void retrieveOwnKeyRequested();

private:
	/**
	 * Enables session building for new devices even before sending a message.
	 */
	void enableSessionBuildingForNewDevices();

	QFuture<void> retrieveOwnKey(QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> keys = {});

	void retrieveDevicesForRequestedJids(const QString &jid);
	void retrieveDevices(const QList<QString> &jids);
	void emitDeviceSignals(const QString &jid, const QList<QString> &distrustedDevices, const QList<QString> &usableDevices, const QList<QString> &authenticatableDevices);

	std::unique_ptr<OmemoDb> m_omemoStorage;
	QXmppOmemoManager *const m_manager;

	bool m_isLoaded = false;
	QList<QString> m_lastRequestedDeviceJids;
};
