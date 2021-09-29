// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TrustDb.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringBuilder>

#include <QXmppTrustMessageKeyOwner.h>

#include "SqlUtils.h"

using namespace QXmpp;
using namespace SqlUtils;

struct Key {
	QString encryption;
	QByteArray keyId;
	QString ownerJid;
	TrustLevel trustLevel;
};

struct UnprocessedKey {
	QString encryption;
	QByteArray keyId;
	QString ownerJid;
	QByteArray senderKeyId;
	bool trust;
};

std::vector<TrustLevel> trustFlagsVec(TrustLevels levels)
{
	std::vector<TrustLevel> vec;
	for (uint i = 0; i < int(sizeof(TrustLevels::Int) * 8); i++) {
		const int level = 1U << i;
		if (levels & level) {
			vec.push_back(TrustLevel(level));
		}
	}
	return vec;
}

QString trustFlagsToString(TrustLevels levels)
{
	const auto levelVec = trustFlagsVec(levels);
	if (levelVec.empty()) {
		return {};
	}
	QStringList levelStrings;
	std::transform(levelVec.begin(), levelVec.end(), std::back_inserter(levelStrings), [](auto level) {
		return QString::number(int(level));
	});
	return levelStrings.join(u", ");
}

TrustDb::TrustDb(Database *database, QString accountJid, QObject *parent)
	: DatabaseComponent(database, parent), m_accountJid(std::move(accountJid))
{
}

auto TrustDb::securityPolicy(const QString &encryption) -> QFuture<SecurityPolicy>
{
	return run([this, encryption] {
		auto query = createQuery();
		execQuery(query,
			"SELECT security_policy FROM trust_security_policies WHERE account = :1 "
			"AND encryption = :2",
			{m_accountJid, encryption});

		if (query.next()) {
			bool ok = false;
			if (const auto result = query.value(0).toUInt(&ok); ok) {
				return SecurityPolicy(result);
			}
		}
		return NoSecurityPolicy;
	});
}

auto TrustDb::setSecurityPolicy(const QString &encryption, SecurityPolicy securityPolicy) -> QFuture<void>
{
	return run([this, encryption, securityPolicy] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO trust_security_policies "
			"VALUES (:1, :2, :3)",
			{m_accountJid, encryption, int(securityPolicy)});
	});
}

auto TrustDb::resetSecurityPolicy(const QString &encryption) -> QFuture<void>
{
	return run([this, encryption] { _resetSecurityPolicy(encryption); });
}

auto TrustDb::ownKey(const QString &encryption) -> QFuture<QByteArray>
{
	return run([this, encryption] {
		auto query = createQuery();
		execQuery(query,
			"SELECT key_id FROM trust_own_keys WHERE account = :1 AND "
			"encryption = :2",
			{m_accountJid, encryption});

		if (query.next()) {
			return query.value(0).toByteArray();
		}
		return QByteArray();
	});
}

auto TrustDb::setOwnKey(const QString &encryption, const QByteArray &keyId) -> QFuture<void>
{
	return run([this, encryption, keyId] {
		auto query = createQuery();
		execQuery(query,
			"INSERT OR REPLACE INTO trust_own_keys VALUES (:1, :2, :3)",
			{m_accountJid, encryption, keyId});
	});
}

auto TrustDb::resetOwnKey(const QString &encryption) -> QFuture<void>
{
	return run([this, encryption] { _resetOwnKey(encryption); });
}

auto TrustDb::keys(const QString &encryption, TrustLevels trustLevels)
	-> QFuture<QHash<TrustLevel, QMultiHash<QString, QByteArray>>>
{
	return run([this, encryption, trustLevels] {
		auto query = createQuery();
		if (trustLevels != 0) {
			// causes possible sql injection, but the output from trustFlagsListString() is safe
			// binding the value is not possible as it would be inserted as a string (we need a condition)
			prepareQuery(query,
				u"SELECT key_id, owner_jid, trust_level FROM trust_keys "
				"WHERE account = :1 AND encryption = :2 "
				"AND trust_level IN (" %
					trustFlagsToString(trustLevels) % u")");
		} else {
			prepareQuery(query,
				"SELECT key_id, owner_jid, trust_level FROM trust_keys WHERE "
				"account = :1 AND encryption = :2");
		}

		// parsing function
		enum { KeyId, OwnerJid, TrustLevel_ };
		struct QueryResult {
			QByteArray keyId;
			QString ownerJid;
			TrustLevel trustLevel;
		};
		auto parseQuery = [](QSqlQuery &query) {
			std::vector<QueryResult> keys;
			while (query.next()) {
				keys.push_back(QueryResult {
					query.value(KeyId).toByteArray(),
					query.value(OwnerJid).toString(),
					TrustLevel(query.value(TrustLevel_).toInt()),
				});
			}
			return keys;
		};

		// execute
		bindValues(query, {m_accountJid, encryption});
		execQuery(query);

		// create output
		QHash<TrustLevel, QMultiHash<QString, QByteArray>> output;
		for (const auto &key : parseQuery(query)) {
			output[key.trustLevel].insert(key.ownerJid, key.keyId);
		}
		return output;
	});
}

auto TrustDb::keys(const QString &encryption, const QList<QString> &keyOwnerJids, TrustLevels trustLevels)
	-> QFuture<QHash<QString, QHash<QByteArray, TrustLevel>>>
{
	Q_ASSERT(!keyOwnerJids.isEmpty());
	return run([this, encryption, trustLevels, keyOwnerJids] {
		auto query = createQuery();
		if (trustLevels != 0) {
			// causes possible sql injection, but the output from trustFlagsListString() is safe
			// binding the value is not possible as it would be inserted as a string (we need a condition)
			const auto trustFlagsCondition = trustFlagsToString(trustLevels);
			prepareQuery(query,
				u"SELECT key_id, trust_level FROM trust_keys "
				"WHERE account = :1 AND encryption = :2 AND owner_jid = :3 AND "
				"trust_level IN (" %
					trustFlagsCondition % u")");
		} else {
			prepareQuery(query,
				"SELECT key_id, trust_level FROM trust_keys WHERE account = :1 AND "
				"encryption = :2 AND owner_jid = :3");
		}

		// parsing function
		enum { KeyId, TrustLevel_ };
		struct QueryResult {
			QByteArray keyId;
			TrustLevel trustLevel;
		};
		auto parseQuery = [](QSqlQuery &query) {
			std::vector<QueryResult> keys;
			while (query.next()) {
				keys.push_back(QueryResult {query.value(KeyId).toByteArray(),
					TrustLevel(query.value(TrustLevel_).toInt())});
			}
			return keys;
		};

		// execute queries with all JIDs and create output
		QHash<QString, QHash<QByteArray, TrustLevel>> output;
		for (const auto &ownerJid : keyOwnerJids) {
			bindValues(query, {m_accountJid, encryption, ownerJid});
			execQuery(query);

			for (const auto &key : parseQuery(query)) {
				output[ownerJid][key.keyId] = key.trustLevel;
			}
		}
		return output;
	});
}

auto TrustDb::addKeys(const QString &encryption,
	const QString &keyOwnerJid,
	const QList<QByteArray> &keyIds,
	TrustLevel trustLevel) -> QFuture<void>
{
	std::vector<Key> keys;
	keys.reserve(keyIds.size());
	std::transform(keyIds.begin(), keyIds.end(), std::back_inserter(keys), [&](const auto &keyId) {
		return Key {encryption, keyId, keyOwnerJid, trustLevel};
	});

	return insertKeys(std::move(keys));
}

auto TrustDb::removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) -> QFuture<void>
{
	return run([=] {
		auto query = createQuery();
		prepareQuery(query,
			"DELETE FROM trust_keys WHERE account = :1 AND encryption = :2 AND "
			"key_id = :3");

		for (const auto &keyId : keyIds) {
			bindValues(query, {m_accountJid, encryption, keyId});
			execQuery(query);
		}
	});
}

auto TrustDb::removeKeys(const QString &encryption, const QString &keyOwnerJid) -> QFuture<void>
{
	return run([=] {
		auto query = createQuery();
		prepareQuery(query,
			"DELETE FROM trust_keys WHERE account = :1 AND encryption = :2 AND "
			"owner_jid = :3");
		bindValues(query, {m_accountJid, encryption, keyOwnerJid});
		execQuery(query);
	});
}

auto TrustDb::removeKeys(const QString &encryption) -> QFuture<void>
{
	return run([=] {
		auto query = createQuery();
		prepareQuery(
			query, "DELETE FROM trust_keys WHERE account = :1 AND encryption = :2");
		bindValues(query, {m_accountJid, encryption});
		execQuery(query);
	});
}

auto TrustDb::hasKey(const QString &encryption, const QString &keyOwnerJid, TrustLevels trustLevels)
	-> QFuture<bool>
{
	Q_ASSERT(int(trustLevels) > 0);
	return run([=] {
		auto query = createQuery();
		execQuery(query,
			u"SELECT COUNT(*) FROM trust_keys "
			u"WHERE account = :1 AND encryption = :2 AND owner_jid = :3 AND "
			u"trust_level IN (" %
				trustFlagsToString(trustLevels) % u")",
			{m_accountJid, encryption, keyOwnerJid});
		if (query.next()) {
			return query.value(0).toInt() > 0;
		}
		return false;
	});
}

auto TrustDb::trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
	-> QFuture<TrustLevel>
{
	return run([this, encryption, keyOwnerJid, keyId] {
		auto query = createQuery();
		execQuery(query,
			"SELECT trust_level FROM trust_keys "
			"WHERE account = :1 AND encryption = :2 AND owner_jid = :3 AND key_id = "
			":4",
			{m_accountJid, encryption, keyOwnerJid, keyId});
		if (query.next()) {
			bool ok = false;
			if (const auto result = query.value(0).toInt(&ok); ok) {
				return TrustLevel(result);
			}
		}
		return TrustLevel::Undecided;
	});
}

auto TrustDb::setTrustLevel(const QString &encryption,
	const QMultiHash<QString, QByteArray> &keyIds,
	TrustLevel trustLevel) -> QFuture<TrustChanges>
{
	return run([this, encryption, keyIds, trustLevel, account = m_accountJid] {
		auto query = createQuery();
		enum { RowId, TrustLevel_ };
		prepareQuery(query,
			"SELECT rowid, trust_level FROM trust_keys "
			"WHERE account = :account AND encryption = :encryption AND "
			"owner_jid = :jid AND key_id = :key_id");

		TrustChanges result;
		auto &changes = result[encryption];

		for (auto itr = keyIds.begin(); itr != keyIds.end(); ++itr) {
			const auto &ownerJid = itr.key();
			const auto &keyId = itr.value();

			bindValues(query,
				{
					{u":account", account},
					{u":encryption", encryption},
					{u":jid", ownerJid},
					{u":key_id", keyId},
				});
			execQuery(query);

			if (query.next()) {
				if (query.value(TrustLevel_).toLongLong() != qint64(trustLevel)) {
					_setTrustLevel(trustLevel, query.value(RowId).toLongLong());
					changes.insert(ownerJid, keyId);
				}
				// added and has correct trust level
			} else {
				// key needs to be added
				insertKeys({Key {encryption, keyId, ownerJid, trustLevel}});
				changes.insert(ownerJid, keyId);
			}
		}
		return result;
	});
}

auto TrustDb::setTrustLevel(const QString &encryption,
	const QList<QString> &keyOwnerJids,
	TrustLevel oldTrustLevel,
	TrustLevel newTrustLevel) -> QFuture<TrustChanges>
{
	return run([this, encryption, keyOwnerJids, oldTrustLevel, newTrustLevel, account = m_accountJid] {
		TrustChanges result;
		auto &changes = result[encryption];
		std::vector<qint64> updateRowIds;

		enum { RowId, KeyId };
		auto query = createQuery();
		prepareQuery(query,
			"SELECT rowid, key_id FROM trust_keys "
			"WHERE account = :account AND encryption = :encryption AND owner_jid = "
			":jid "
			"AND trust_level = :old_trust");

		for (const auto &jid : keyOwnerJids) {
			bindValues(query,
				{
					{u":account", account},
					{u":encryption", encryption},
					{u":jid", jid},
					{u":old_trust", int(oldTrustLevel)},
				});
			execQuery(query);

			while (query.next()) {
				updateRowIds.push_back(query.value(RowId).toLongLong());
				changes.insert(jid, query.value(KeyId).toByteArray());
			}
		}

		for (auto rowId : updateRowIds) {
			_setTrustLevel(newTrustLevel, rowId);
		}

		return result;
	});
}

auto TrustDb::addKeysForPostponedTrustDecisions(const QString &encryption,
	const QByteArray &senderKeyId,
	const QList<QXmppTrustMessageKeyOwner> &keyOwners) -> QFuture<void>
{
	std::vector<UnprocessedKey> keys;
	keys.reserve(std::accumulate(
		keyOwners.begin(), keyOwners.end(), std::size_t(0), [](std::size_t sum, const auto &keyOwner) {
			return sum + keyOwner.trustedKeys().size() + keyOwner.distrustedKeys().size();
		}));

	for (const auto &keyOwner : keyOwners) {
		const auto trustKeys = keyOwner.trustedKeys();
		const auto distrustKeys = keyOwner.distrustedKeys();
		for (const auto &keyId : trustKeys) {
			keys.push_back(UnprocessedKey {encryption, keyId, keyOwner.jid(), senderKeyId, true});
		}
		for (const auto &keyId : distrustKeys) {
			keys.push_back(UnprocessedKey {encryption, keyId, keyOwner.jid(), senderKeyId, false});
		}
	}

	return run([this, keys = std::move(keys)] {
		transaction();
		auto query = createQuery();
		prepareQuery(query,
			"INSERT OR REPLACE INTO trust_keys_unprocessed "
			"(account, encryption, key_id, owner_jid, sender_key_id, trust) "
			"VALUES (:1, :2, :3, :4, :5, :6)");
		for (const auto &key : keys) {
			bindValues(query,
				{m_accountJid, key.encryption, key.keyId, key.ownerJid, key.senderKeyId, key.trust});
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption,
	const QList<QByteArray> &keyIdsForAuthentication,
	const QList<QByteArray> &keyIdsForDistrusting) -> QFuture<void>
{
	using namespace std::placeholders;
	struct Selector {
		QString encryption;
		QByteArray keyId;
		bool trust;
	};
	auto toSelector = [&](bool trust, const QByteArray &keyId) {
		return Selector {encryption, keyId, trust};
	};

	std::vector<Selector> selectors;
	selectors.reserve(keyIdsForAuthentication.size() + keyIdsForDistrusting.size());
	std::transform(keyIdsForAuthentication.begin(),
		keyIdsForAuthentication.end(),
		std::back_inserter(selectors),
		[toSelector](const auto &keyId) { return toSelector(true, keyId); });
	std::transform(keyIdsForDistrusting.begin(),
		keyIdsForDistrusting.end(),
		std::back_inserter(selectors),
		[toSelector](const auto &keyId) { return toSelector(false, keyId); });

	return run([this, selectors = std::move(selectors)] {
		transaction();
		auto query = createQuery();
		prepareQuery(query,
			"DELETE FROM trust_keys_unprocessed WHERE account = :1 AND encryption = :2 "
			"AND key_id = :3 AND trust = :4");
		for (const auto &selector : selectors) {
			bindValues(query, {m_accountJid, selector.encryption, selector.keyId, selector.trust});
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption,
	const QList<QByteArray> &senderKeyIds) -> QFuture<void>
{
	return run([=] {
		transaction();
		auto query = createQuery();
		prepareQuery(query,
			"DELETE FROM trust_keys_unprocessed WHERE account = :1 AND encryption = :2 "
			"AND sender_key_id = :3");
		for (const auto &keyId : senderKeyIds) {
			bindValues(query, {m_accountJid, encryption, keyId});
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption) -> QFuture<void>
{
	return run([this, encryption] {
		auto query = createQuery();
		execQuery(query,
			"DELETE FROM trust_keys_unprocessed WHERE account = :1 AND encryption = :2",
			{m_accountJid, encryption});
	});
}

auto TrustDb::keysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &senderKeyIds)
	-> QFuture<QHash<bool, QMultiHash<QString, QByteArray>>>
{
	if (senderKeyIds.isEmpty()) {
		return run([=] {
			QHash<bool, QMultiHash<QString, QByteArray>> result;

			auto query = createQuery();
			prepareQuery(query,
				"SELECT key_id, owner_jid, trust FROM trust_keys_unprocessed WHERE "
				"account = :1 AND encryption = :2");
			bindValues(query, {m_accountJid, encryption});
			execQuery(query);
			enum { KeyId, OwnerJid, Trust };
			while (query.next()) {
				result[query.value(Trust).toBool()].insert(query.value(OwnerJid).toString(),
					query.value(KeyId).toByteArray());
			}

			return result;
		});
	}
	return run([=] {
		enum { KeyId, OwnerJid, Trust };
		QHash<bool, QMultiHash<QString, QByteArray>> result;

		auto query = createQuery();
		prepareQuery(query,
			"SELECT key_id, owner_jid, trust FROM trust_keys_unprocessed WHERE account = :1 "
			"AND encryption = :2 AND sender_key_id = :3");

		for (const auto &senderKeyId : senderKeyIds) {
			bindValues(query, {m_accountJid, encryption, senderKeyId});
			execQuery(query);
			while (query.next()) {
				result[query.value(Trust).toBool()].insert(query.value(OwnerJid).toString(),
					query.value(KeyId).toByteArray());
			}
		}

		return result;
	});
}

auto TrustDb::resetAll(const QString &encryption) -> QFuture<void>
{
	return run([=] {
		_resetSecurityPolicy(encryption);
		_resetOwnKey(encryption);

		auto query = createQuery();
		execQuery(query,
			"DELETE FROM trust_keys WHERE account = :1 AND encryption = :2",
			{m_accountJid, encryption});
		execQuery(query,
			"DELETE FROM trust_keys_unprocessed WHERE account = :1 AND encryption = :2",
			{m_accountJid, encryption});
	});
}

auto TrustDb::resetAll() -> QFuture<void>
{
	return run([this] {
		auto query = createQuery();
		execQuery(query, "DELETE FROM trust_security_policies WHERE account = :1", {m_accountJid});
		execQuery(query, "DELETE FROM trust_own_keys WHERE account = :1", {m_accountJid});
		execQuery(query, "DELETE FROM trust_keys WHERE account = :1", {m_accountJid});
		execQuery(query, "DELETE FROM trust_keys_unprocessed WHERE account = :1", {m_accountJid});
	});
}

auto TrustDb::insertKeys(std::vector<Key> &&keys) -> QFuture<void>
{
	return run([this, keys = std::move(keys)] {
		transaction();
		auto query = createQuery();
		prepareQuery(query,
			"INSERT OR REPLACE INTO trust_keys (account, encryption, key_id, "
			"owner_jid, "
			"trust_level) VALUES (:1, :2, :3, :4, :5)");

		for (const auto &key : keys) {
			bindValues(query,
				{m_accountJid, key.encryption, key.keyId, key.ownerJid, int(key.trustLevel)});
			execQuery(query);
		}
		commit();
	});
}

void TrustDb::_resetSecurityPolicy(const QString &encryption)
{
	auto query = createQuery();
	execQuery(query,
		"DELETE FROM trust_security_policies WHERE account = :1 AND encryption = :2",
		{m_accountJid, encryption});
}

void TrustDb::_resetOwnKey(const QString &encryption)
{
	auto query = createQuery();
	execQuery(query,
		"DELETE FROM trust_own_keys WHERE account = :1 AND encryption = :2",
		{m_accountJid, encryption});
}

void TrustDb::_setTrustLevel(TrustLevel trustLevel, qint64 rowId)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(query, "UPDATE trust_keys SET trust_level = ? WHERE rowid = ?");
		return query;
	}();

	bindValues(query, {int(trustLevel), rowId});
	execQuery(query);
	Q_ASSERT(query.numRowsAffected() == 1);
	query.finish();
}
