/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2020 Kaidan developers and contributors
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

#ifndef REGISTRATIONMANAGER_H
#define REGISTRATIONMANAGER_H

#include <QXmppClientExtension.h>
class ClientWorker;
class Kaidan;
class QSettings;
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 2, 0)
class QXmppRegistrationManager;
#endif

class RegistrationManager : public QXmppClientExtension
{
	Q_OBJECT
	Q_PROPERTY(bool registrationSupported READ registrationSupported
	                                      NOTIFY registrationSupportedChanged)

public:
	RegistrationManager(Kaidan *kaidan, ClientWorker *clientWorker, QXmppClient *client, QSettings *settings);

	QStringList discoveryFeatures() const override;

	/**
	 * @brief Changes the user's password
	 * @param newPassword The requested new password
	 */
	void changePassword(const QString &newPassword);

	bool registrationSupported() const;

public slots:
	/**
	 * Deletes the account from the server.
	 */
	void deleteAccount();

signals:
	void passwordChanged(const QString &newPassword);
	void passwordChangeFailed();
	void registrationSupportedChanged();

protected:
	void setClient(QXmppClient *client) override;

private slots:
	void handleDiscoInfo(const QXmppDiscoveryIq &iq);

private:
	bool handleStanza(const QDomElement &stanza) override;
	void setRegistrationSupported(bool registrationSupported);

	Kaidan *kaidan;
	ClientWorker *clientWorker;
	QSettings *settings;
	QXmppClient *m_client;
#if (QXMPP_VERSION) >= QT_VERSION_CHECK(1, 2, 0)
	QXmppRegistrationManager *m_manager;
#endif
	bool m_registrationSupported = false;

	// caching
	QString m_newPasswordIqId;
	QString m_newPassword;
};

#endif // REGISTRATIONMANAGER_H
