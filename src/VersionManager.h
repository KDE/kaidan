// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class QXmppClient;
class QXmppVersionIq;
class QXmppVersionManager;

class VersionManager : public QObject
{
	Q_OBJECT

public:
	explicit VersionManager(QXmppClient *client, QObject *parent = nullptr);

	/**
	 * Fetches the version information of all resources of the given  bare JID
	 */
	Q_INVOKABLE void fetchVersions(const QString &bareJid, const QString &resource);

signals:
	/**
	 * Emitted when a client version information was received
	 */
	void clientVersionReceived(const QXmppVersionIq &versionIq);

private:
	QXmppVersionManager *m_manager;
	QXmppClient *m_client;
};
