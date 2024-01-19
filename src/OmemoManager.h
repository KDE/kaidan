// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFuture>

#include <QXmppTask.h>
#include <QXmppTrustLevel.h>

class Database;
class OmemoDb;
class QXmppClient;
class QXmppOmemoManager;

class OmemoManager : public QObject
{
	Q_OBJECT

public:
	struct Device {
		QString label;
		QString keyId;

		bool operator==(const Device &other) const = default;
	};

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
	QFuture<bool> hasUsableDevices(const QList<QString> &jids);

	QFuture<void> requestDeviceLists(const QList<QString> &jids);
	QFuture<void> subscribeToDeviceLists(const QList<QString> &jids);
	QFuture<void> unsubscribeFromDeviceLists();
	QXmppTask<bool> resetOwnDevice();

	QFuture<void> initializeChat(const QString &accountJid, const QString &chatJid);
	void removeContactDevices(const QString &jid);

	Q_SIGNAL void initializeChatRequested(const QString &accountJid);
	Q_SIGNAL void retrieveOwnKeyRequested();

private:
	/**
	 * Enables session building for new devices even before sending a message.
	 */
	void enableSessionBuildingForNewDevices();

	QFuture<void> retrieveOwnKey(QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> keys = {});

	void retrieveKeysForRequestedJids(const QList<QString> &jids);
	void retrieveDevicesForRequestedJids(const QString &jid);

	void retrieveDevices(const QList<QString> &jids);

	void updateCachedKeys(const QString &jid, const QList<QString> &authenticatableKeys, const QList<QString> &authenticatedKeys);
	void updateCachedDevices(const QString &jid, const QList<Device> &distrustedDevices, const QList<Device> &usableDevices, const QList<Device> &authenticatableDevices, const QList<Device> &authenticatedDevices);

	std::unique_ptr<OmemoDb> m_omemoStorage;
	QXmppOmemoManager *const m_manager;

	bool m_isLoaded = false;
	QList<QString> m_lastRequestedKeyOwnerJids;
};

Q_DECLARE_METATYPE(OmemoManager::Device)
