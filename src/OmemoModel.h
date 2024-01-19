// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
#include <QFuture>
#include <QVector>
// Kaidan
#include "OmemoCache.h"

class OmemoModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QString jid MEMBER m_jid WRITE setJid)
	Q_PROPERTY(bool ownAuthenticatedKeysProcessed MEMBER m_ownAuthenticatedKeysProcessed WRITE setOwnAuthenticatedKeysProcessed)

public:
	enum class Role {
		Label = Qt::DisplayRole,
		KeyId,
	};

	explicit OmemoModel(QObject *parent = nullptr);
	~OmemoModel() override;

	[[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;
	[[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

	void setJid(const QString &jid);
	void setOwnAuthenticatedKeysProcessed(bool ownAuthenticatedKeysProcessed);

	Q_INVOKABLE bool contains(const QString &keyId);

private:
	void setUp();

	void setOwnDevice(OmemoManager::Device ownDevice);
	void setDevices(const QList<OmemoManager::Device> &devices);

	QString m_jid;
	bool m_ownAuthenticatedKeysProcessed = false;

	std::optional<OmemoManager::Device> m_ownDevice;
	QList<OmemoManager::Device> m_devices;
};
