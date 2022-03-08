// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DatabaseComponent.h"
#include <QXmppOmemoStorage.h>

class OmemoDb : public DatabaseComponent, public QXmppOmemoStorage
{
public:
	using SignedPreKeyPairs = QHash<uint32_t, SignedPreKeyPair>;
	using PreKeyPairs = QHash<uint32_t, QByteArray>;
	using Devices = QHash<QString, QHash<uint32_t, Device>>;

	OmemoDb(Database *db, QString accountJid, QObject *parent = nullptr);

	inline auto accountJid() const
	{
		return m_accountJid;
	}
	inline void setAccountJid(QString accountJid)
	{
		m_accountJid = std::move(accountJid);
	};

	auto allData() -> QFuture<OmemoData> override;
	auto resetAll() -> QFuture<void> override;

	auto setOwnDevice(const std::optional<OwnDevice> &device) -> QFuture<void> override;

	auto addSignedPreKeyPair(uint32_t keyId, const SignedPreKeyPair &keyPair) -> QFuture<void> override;
	auto removeSignedPreKeyPair(uint32_t keyId) -> QFuture<void> override;

	auto addPreKeyPairs(const QHash<uint32_t, QByteArray> &keyPairs) -> QFuture<void> override;
	auto removePreKeyPair(uint32_t keyId) -> QFuture<void> override;

	auto addDevice(const QString &jid, uint32_t deviceId, const Device &device) -> QFuture<void> override;
	auto removeDevice(const QString &jid, uint32_t deviceId) -> QFuture<void> override;
	auto removeDevices(const QString &jid) -> QFuture<void> override;

private:
	auto _ownDevice() -> std::optional<OwnDevice>;
	auto _signedPreKeyPairs() -> SignedPreKeyPairs;
	auto _preKeyPairs() -> PreKeyPairs;
	auto _devices() -> Devices;

	QString m_accountJid;
};
