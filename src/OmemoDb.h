// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
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

	OmemoDb(Database *db, QObject *xmppContext, QString accountJid, QObject *parent = nullptr);

	inline auto accountJid() const
	{
		return m_accountJid;
	}
	inline void setAccountJid(QString accountJid)
	{
		m_accountJid = std::move(accountJid);
	};

	auto allData() -> QXmppTask<OmemoData> override;
	auto resetAll() -> QXmppTask<void> override;

	auto setOwnDevice(const std::optional<OwnDevice> &device) -> QXmppTask<void> override;

	auto addSignedPreKeyPair(uint32_t keyId, const SignedPreKeyPair &keyPair) -> QXmppTask<void> override;
	auto removeSignedPreKeyPair(uint32_t keyId) -> QXmppTask<void> override;

	auto addPreKeyPairs(const QHash<uint32_t, QByteArray> &keyPairs) -> QXmppTask<void> override;
	auto removePreKeyPair(uint32_t keyId) -> QXmppTask<void> override;

	auto addDevice(const QString &jid, uint32_t deviceId, const Device &device) -> QXmppTask<void> override;
	auto removeDevice(const QString &jid, uint32_t deviceId) -> QXmppTask<void> override;
	auto removeDevices(const QString &jid) -> QXmppTask<void> override;

private:
	template<typename Functor>
	auto runTask(Functor function) const
	{
		return runAsyncTask(m_xmppContext, dbWorker(), function);
	}

	auto _ownDevice() -> std::optional<OwnDevice>;
	auto _signedPreKeyPairs() -> SignedPreKeyPairs;
	auto _preKeyPairs() -> PreKeyPairs;
	auto _devices() -> Devices;

	QObject *m_xmppContext;
	QString m_accountJid;
};
