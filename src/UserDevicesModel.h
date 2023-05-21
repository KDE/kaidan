// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>
#include <QVector>

#include "PresenceCache.h"

class QXmppVersionIq;

class UserDevicesModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)

public:
	enum Roles {
		Resource = Qt::UserRole + 1,
		Name,
		Version,
		OS
	};

	explicit UserDevicesModel(QObject *parent = nullptr);

	QHash<int, QByteArray> roleNames() const override;
	QVariant data(const QModelIndex &index, int role) const override;
	int rowCount(const QModelIndex &parent) const override;

	QString jid() const;
	void setJid(const QString &jid);

signals:
	void jidChanged();
	void clientVersionsRequested(const QString &bareJid, const QString &resource = {});

private:
	void handleClientVersionReceived(const QXmppVersionIq &versionIq);
	void handlePresenceChanged(PresenceCache::ChangeType type,
	                           const QString &jid,
	                           const QString &resource);
	void handlePresencesCleared();

	struct DeviceInfo {
		DeviceInfo(const QString &resource);
		DeviceInfo(const QXmppVersionIq &);

		QString resource;
		QString name;
		QString version;
		QString os;
	};

	QString m_jid;
	QVector<DeviceInfo> m_devices;
};
