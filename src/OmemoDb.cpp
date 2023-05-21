// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoDb.h"
#include "Globals.h"
#include "QXmppFutureUtils_p.h"
#include "SqlUtils.h"
#include <QSqlQuery>
#include <QStringBuilder>

using namespace SqlUtils;

constexpr std::initializer_list<QStringView> OMEMO_TABLES = {u"omemoDevicesOwn",
	u"omemoDevices",
	u"omemoPreKeyPairs",
	u"omemoPreKeyPairsSigned"};

OmemoDb::OmemoDb(Database *db, QObject *xmppContext, QString accountJid, QObject *parent)
	: DatabaseComponent(db, parent),
	  m_xmppContext(xmppContext),
	  m_accountJid(std::move(accountJid))
{
}

auto OmemoDb::allData() -> QXmppTask<OmemoData>
{
	return runTask([this] {
		return OmemoData {
			.ownDevice = _ownDevice(),
			.signedPreKeyPairs = _signedPreKeyPairs(),
			.preKeyPairs = _preKeyPairs(),
			.devices = _devices(),
		};
	});
}

auto OmemoDb::resetAll() -> QXmppTask<void>
{
	return runTask([this] {
		auto query = createQuery();
		for (auto table : OMEMO_TABLES) {
			execQuery(query, u"DELETE FROM " % table % u" WHERE account = ?", {accountJid()});
		}
	});
}

auto OmemoDb::setOwnDevice(const std::optional<OwnDevice> &device) -> QXmppTask<void>
{
	if (device) {
		// set new device
		return runTask([this, device = *device] {
			auto query = createQuery();
			prepareQuery(query,
				"INSERT OR REPLACE INTO omemoDevicesOwn "
				"(account, id, label, privateKey, publicKey, "
				"latestSignedPreKeyId, latestPreKeyId) "
				"VALUES (?, ?, ?, ?, ?, ?, ?)");
			bindValues(query,
				{accountJid(),
					device.id,
					device.label,
					device.privateIdentityKey,
					device.publicIdentityKey,
					device.latestSignedPreKeyId,
					device.latestPreKeyId});
			execQuery(query);
		});
	}
	// remove old own device
	return runTask([this] {
		auto query = createQuery();
		execQuery(query, "DELETE FROM omemoDevicesOwn WHERE account = ?", {accountJid()});
	});
}

auto OmemoDb::_ownDevice() -> std::optional<OwnDevice>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT id, label, privateKey, publicKey, latestSignedPreKeyId, "
		"latestPreKeyId FROM omemoDevicesOwn WHERE account = ?",
		{accountJid()});

	enum { Id, Label, PrivateKey, PublicKey, LatestSignedPreKeyId, LatestPreKeyId };
	auto parse = [&query] {
		return OwnDevice {
			.id = query.value(Id).toUInt(),
			.label = query.value(Label).toString(),
			.privateIdentityKey = query.value(PrivateKey).toByteArray(),
			.publicIdentityKey = query.value(PublicKey).toByteArray(),
			.latestSignedPreKeyId = query.value(LatestSignedPreKeyId).toUInt(),
			.latestPreKeyId = query.value(LatestPreKeyId).toUInt(),
		};
	};
	if (query.next()) {
		return parse();
	}
	return std::nullopt;
}

auto OmemoDb::addSignedPreKeyPair(uint32_t keyId, const SignedPreKeyPair &keyPair) -> QXmppTask<void>
{
	return runTask([this, keyId, keyPair] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO omemoPreKeyPairsSigned (account, id, data, "
			"creationTimestamp) "
			"VALUES (?, ?, ?, ?)",
			{accountJid(), keyId, keyPair.data, serialize(keyPair.creationDate)});
	});
}

auto OmemoDb::removeSignedPreKeyPair(uint32_t keyId) -> QXmppTask<void>
{
	return runTask([this, keyId] {
		auto query = createQuery();
		execQuery(query,
			"DELETE FROM omemoPreKeyPairsSigned WHERE account = ? AND id = ?",
			{accountJid(), keyId});
	});
}

auto OmemoDb::_signedPreKeyPairs() -> QHash<uint32_t, SignedPreKeyPair>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT id, data, creationTimestamp FROM omemoPreKeyPairsSigned "
		"WHERE account = ?",
		{accountJid()});

	enum { Id, Data, CreationTimestamp };
	auto parse = [](auto &query) {
		return SignedPreKeyPair {
			.creationDate = parseDateTime(query, CreationTimestamp),
			.data = query.value(Data).toByteArray(),
		};
	};

	QHash<uint32_t, SignedPreKeyPair> output;
	while (query.next()) {
		output.insert(query.value(Id).toUInt(), parse(query));
	}
	return output;
}

auto OmemoDb::addPreKeyPairs(const QHash<uint32_t, QByteArray> &keyPairs) -> QXmppTask<void>
{
	return runTask([this, keyPairs] {
		auto query = createQuery();
		prepareQuery(query,
			"INSERT OR REPLACE INTO omemoPreKeyPairs (account, id, data) "
			"VALUES (?, ?, ?)");

		for (auto itr = keyPairs.begin(); itr != keyPairs.end(); itr++) {
			bindValues(query, {accountJid(), itr.key(), itr.value()});
			execQuery(query);
		}
	});
}

auto OmemoDb::removePreKeyPair(uint32_t keyId) -> QXmppTask<void>
{
	return runTask([this, keyId] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral("DELETE FROM omemoPreKeyPairs WHERE account = ? AND id = ?"),
			{accountJid(), keyId});
	});
}

auto OmemoDb::_preKeyPairs() -> QHash<uint32_t, QByteArray>
{
	auto query = createQuery();
	execQuery(query, QStringLiteral("SELECT id, data FROM omemoPreKeyPairs WHERE account = ?"), {accountJid()});

	enum { Id, Data };
	QHash<uint32_t, QByteArray> output;
	while (query.next()) {
		output.insert(query.value(Id).toUInt(), query.value(Data).toByteArray());
	}
	return output;
}

auto OmemoDb::addDevice(const QString &jid, uint32_t deviceId, const Device &dev) -> QXmppTask<void>
{
	return runTask([this, jid, deviceId, dev] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO omemoDevices (account, userJid, id, "
			"label, keyId, session, unrespondedStanzasSent, "
			"unrespondedStanzasReceived, removalTimestamp) "
			"VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?)",
			{accountJid(),
				jid,
				deviceId,
				dev.label,
				dev.keyId,
				dev.session,
				dev.unrespondedSentStanzasCount,
				dev.unrespondedReceivedStanzasCount,
				serialize(dev.removalFromDeviceListDate)});
	});
}

auto OmemoDb::removeDevice(const QString &jid, uint32_t deviceId) -> QXmppTask<void>
{
	return runTask([this, jid, deviceId] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral("DELETE FROM omemoDevices WHERE account = ? AND userJid = "
				       "? AND id = ?"),
			{accountJid(), jid, deviceId});
	});
}

auto OmemoDb::removeDevices(const QString &jid) -> QXmppTask<void>
{
	return runTask([this, jid] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral(
				"DELETE FROM omemoDevices WHERE account = ? AND userJid = ?"),
			{accountJid(), jid});
	});
}

auto OmemoDb::_devices() -> QHash<QString, QHash<uint32_t, Device>>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT userJid, id, label, keyId, session, unrespondedStanzasSent, "
		"unrespondedStanzasReceived, removalTimestamp "
		"FROM omemoDevices "
		"WHERE account = ?",
		{accountJid()});

	enum { UserJid, Id, Label, KeyId, Session, UnrespondedSent, UnrespondedRecv, RemovalTimestamp };
	auto parse = [&query] {
		return Device {
			.label = query.value(Label).toString(),
			.keyId = query.value(KeyId).toByteArray(),
			.session = query.value(Session).toByteArray(),
			.unrespondedSentStanzasCount = query.value(UnrespondedSent).toInt(),
			.unrespondedReceivedStanzasCount = query.value(UnrespondedRecv).toInt(),
			.removalFromDeviceListDate = parseDateTime(query, RemovalTimestamp),
		};
	};
	QHash<QString, QHash<uint32_t, Device>> output;
	while (query.next()) {
		output[query.value(UserJid).toString()][query.value(Id).toUInt()] = parse();
	}
	return output;
}
