// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoDb.h"
#include "Globals.h"
#include "SqlUtils.h"
#include <QSqlQuery>
#include <QStringBuilder>

using namespace SqlUtils;

constexpr std::initializer_list<QStringView> OMEMO_TABLES = {u"omemo_devices_own",
	u"omemo_devices",
	u"omemo_pre_key_pairs",
	u"omemo_pre_key_pairs_signed"};

OmemoDb::OmemoDb(Database *db, QString accountJid, QObject *parent)
	: DatabaseComponent(db, parent), m_accountJid(std::move(accountJid))
{
}

auto OmemoDb::allData() -> QFuture<OmemoData>
{
	return run([this] {
		return OmemoData {
			.ownDevice = _ownDevice(),
			.signedPreKeyPairs = _signedPreKeyPairs(),
			.preKeyPairs = _preKeyPairs(),
			.devices = _devices(),
		};
	});
}

auto OmemoDb::resetAll() -> QFuture<void>
{
	return run([this] {
		auto query = createQuery();
		for (auto table : OMEMO_TABLES) {
			execQuery(query, u"DELETE FROM " % table % u" WHERE account = ?", {accountJid()});
		}
	});
}

auto OmemoDb::setOwnDevice(const std::optional<OwnDevice> &device) -> QFuture<void>
{
	if (device) {
		// set new device
		return run([this, device = *device] {
			auto query = createQuery();
			prepareQuery(query,
				"INSERT OR REPLACE INTO omemo_devices_own "
				"(account, id, label, private_key, public_key, "
				"latest_signed_pre_key_id, latest_pre_key_id) "
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
	return run([this] {
		auto query = createQuery();
		execQuery(query, "DELETE FROM omemo_devices_own WHERE account = ?", {accountJid()});
	});
}

auto OmemoDb::_ownDevice() -> std::optional<OwnDevice>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT id, label, private_key, public_key, latest_signed_pre_key_id, "
		"latest_pre_key_id FROM omemo_devices_own WHERE account = ?",
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

auto OmemoDb::addSignedPreKeyPair(uint32_t keyId, const SignedPreKeyPair &keyPair) -> QFuture<void>
{
	return run([this, keyId, keyPair] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO omemo_pre_key_pairs_signed (account, id, data, "
			"creation_timestamp) "
			"VALUES (?, ?, ?, ?)",
			{accountJid(), keyId, keyPair.data, serialize(keyPair.creationDate)});
	});
}

auto OmemoDb::removeSignedPreKeyPair(uint32_t keyId) -> QFuture<void>
{
	return run([this, keyId] {
		auto query = createQuery();
		execQuery(query,
			"DELETE FROM omemo_pre_key_pairs_signed WHERE account = ? AND id = ?",
			{accountJid(), keyId});
	});
}

auto OmemoDb::_signedPreKeyPairs() -> QHash<uint32_t, SignedPreKeyPair>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT id, data, creation_timestamp FROM omemo_pre_key_pairs_signed "
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

auto OmemoDb::addPreKeyPairs(const QHash<uint32_t, QByteArray> &keyPairs) -> QFuture<void>
{
	return run([this, keyPairs] {
		auto query = createQuery();
		prepareQuery(query,
			"INSERT OR REPLACE INTO omemo_pre_key_pairs (account, id, data) "
			"VALUES (?, ?, ?)");

		for (auto itr = keyPairs.begin(); itr != keyPairs.end(); itr++) {
			bindValues(query, {accountJid(), itr.key(), itr.value()});
			execQuery(query);
		}
	});
}

auto OmemoDb::removePreKeyPair(uint32_t keyId) -> QFuture<void>
{
	return run([this, keyId] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral(
				"DELETE FROM omemo_pre_key_pairs WHERE account = ? AND id = ?"),
			{accountJid(), keyId});
	});
}

auto OmemoDb::_preKeyPairs() -> QHash<uint32_t, QByteArray>
{
	auto query = createQuery();
	execQuery(query,
		QStringLiteral("SELECT id, data FROM omemo_pre_key_pairs WHERE account = ?"),
		{accountJid()});

	enum { Id, Data };
	QHash<uint32_t, QByteArray> output;
	while (query.next()) {
		output.insert(query.value(Id).toUInt(), query.value(Data).toByteArray());
	}
	return output;
}

auto OmemoDb::addDevice(const QString &jid, uint32_t deviceId, const Device &dev) -> QFuture<void>
{
	return run([this, jid, deviceId, dev] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO omemo_devices (account, user_jid, id, "
			"label, key_id, session, unresponded_stanzas_sent, "
			"unresponded_stanzas_received, removal_timestamp) "
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

auto OmemoDb::removeDevice(const QString &jid, uint32_t deviceId) -> QFuture<void>
{
	return run([this, jid, deviceId] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral("DELETE FROM omemo_devices WHERE account = ? AND user_jid = "
				       "? AND id = ?"),
			{accountJid(), jid, deviceId});
	});
}

auto OmemoDb::removeDevices(const QString &jid) -> QFuture<void>
{
	return run([this, jid] {
		auto query = createQuery();
		execQuery(query,
			QStringLiteral(
				"DELETE FROM omemo_devices WHERE account = ? AND user_jid = ?"),
			{accountJid(), jid});
	});
}

auto OmemoDb::_devices() -> QHash<QString, QHash<uint32_t, Device>>
{
	auto query = createQuery();
	execQuery(query,
		"SELECT user_jid, id, label, key_id, session, unresponded_stanzas_sent, "
		"unresponded_stanzas_received, removal_timestamp "
		"FROM omemo_devices "
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
