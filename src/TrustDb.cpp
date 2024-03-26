// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TrustDb.h"

#include <QSqlQuery>
#include <QSqlRecord>
#include <QStringBuilder>

#include <QXmppTrustMessageKeyOwner.h>

#include "Globals.h"
#include "SqlUtils.h"

Q_DECLARE_METATYPE(QXmpp::TrustLevel)

using namespace QXmpp;
using namespace SqlUtils;

constexpr auto TRUST_DB_TABLES = {
	DB_TABLE_TRUST_SECURITY_POLICIES,
	DB_TABLE_TRUST_OWN_KEYS,
	DB_TABLE_TRUST_KEYS,
	DB_TABLE_TRUST_KEYS_UNPROCESSED,
};

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

TrustDb::TrustDb(Database *database, QObject *xmppContext, QString accountJid, QObject *parent)
	: DatabaseComponent(database, parent),
	  m_xmppContext(xmppContext),
	  m_accountJid(std::move(accountJid))
{
}

auto TrustDb::securityPolicy(const QString &encryption) -> QXmppTask<SecurityPolicy>
{
	return runTask([this, encryption] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT securityPolicy
				FROM trustSecurityPolicies
				WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);

		if (query.next()) {
			bool ok = false;
			if (const auto result = query.value(0).toUInt(&ok); ok) {
				return SecurityPolicy(result);
			}
		}
		return NoSecurityPolicy;
	});
}

auto TrustDb::setSecurityPolicy(const QString &encryption, SecurityPolicy securityPolicy) -> QXmppTask<void>
{
	return runTask([this, encryption, securityPolicy] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO trustSecurityPolicies (
					account,
					encryption,
					securityPolicy
				)
				VALUES (
					:accountJid,
					:encryption,
					:securityPolicy
				)
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
				{ u":securityPolicy", int(securityPolicy) },
			}
		);
	});
}

auto TrustDb::resetSecurityPolicy(const QString &encryption) -> QXmppTask<void>
{
	return runTask([this, encryption] { _resetSecurityPolicy(encryption); });
}

auto TrustDb::ownKey(const QString &encryption) -> QXmppTask<QByteArray>
{
	return runTask([this, encryption] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT keyId
				FROM trustOwnKeys
				WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);

		if (query.next()) {
			return query.value(0).toByteArray();
		}
		return QByteArray();
	});
}

auto TrustDb::setOwnKey(const QString &encryption, const QByteArray &keyId) -> QXmppTask<void>
{
	return runTask([this, encryption, keyId] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO trustOwnKeys (
					account,
					encryption,
					keyId
				)
				VALUES (
					:account,
					:encryption,
					:keyId
				)
			)"),
			{
				{ u":account", m_accountJid },
				{ u":encryption", encryption },
				{ u":keyId", keyId },
			}
		);
	});
}

auto TrustDb::resetOwnKey(const QString &encryption) -> QXmppTask<void>
{
	return runTask([this, encryption] { _resetOwnKey(encryption); });
}

auto TrustDb::keys(const QString &encryption, TrustLevels trustLevels)
    -> QXmppTask<QHash<TrustLevel, QMultiHash<QString, QByteArray>>>
{
	return runTask([this, encryption, trustLevels] {
		auto query = createQuery();
		if (trustLevels != 0) {
			// causes possible sql injection, but the output from trustFlagsListString() is safe
			// binding the value is not possible as it would be inserted as a string (we need a condition)
			prepareQuery(
				query,
				QStringLiteral(R"(
					SELECT keyId, ownerJid, trustLevel
					FROM trustKeys
					WHERE account = :accountJid AND encryption = :encryption AND trustLevel IN (%1)
				)").arg(trustFlagsToString(trustLevels))
			);
		} else {
			prepareQuery(
				query,
				QStringLiteral(R"(
					SELECT keyId, ownerJid, trustLevel
					FROM trustKeys
					WHERE account = :accountJid AND encryption = :encryption
				)")
			);
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
					query.value(TrustLevel_).value<TrustLevel>(),
				});
			}
			return keys;
		};

		// execute
		bindValues(
			query,
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);
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
	-> QXmppTask<QHash<QString, QHash<QByteArray, TrustLevel>>>
{
	Q_ASSERT(!keyOwnerJids.isEmpty());
	return runTask([this, encryption, trustLevels, keyOwnerJids] {
		auto query = createQuery();
		if (trustLevels != 0) {
			// causes possible sql injection, but the output from trustFlagsListString() is safe
			// binding the value is not possible as it would be inserted as a string (we need a condition)
			prepareQuery(
				query,
				QStringLiteral(R"(
					SELECT keyId, trustLevel
					FROM trustKeys
					WHERE account = :accountJid AND encryption = :encryption AND ownerJid = :ownerJid AND trustLevel IN (%1)
				)").arg(trustFlagsToString(trustLevels))
			);
		} else {
			prepareQuery(
				query,
				QStringLiteral(R"(
					SELECT keyId, trustLevel
					FROM trustKeys
					WHERE account = :accountJid AND encryption = :encryption AND ownerJid = :ownerJid
				)")
			);
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
					query.value(TrustLevel_).value<TrustLevel>()});
			}
			return keys;
		};

		// execute queries with all JIDs and create output
		QHash<QString, QHash<QByteArray, TrustLevel>> output;
		for (const auto &ownerJid : keyOwnerJids) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", encryption },
					{ u":ownerJid", ownerJid },
				}
			);
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
	TrustLevel trustLevel) -> QXmppTask<void>
{
	std::vector<Key> keys;
	keys.reserve(keyIds.size());
	std::transform(keyIds.begin(), keyIds.end(), std::back_inserter(keys), [&](const auto &keyId) {
		return Key {encryption, keyId, keyOwnerJid, trustLevel};
	});

	return insertKeys(std::move(keys));
}

auto TrustDb::removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) -> QXmppTask<void>
{
	return runTask([this, encryption, keyIds] {
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption AND keyId = :keyId
			)")
		);

		for (const auto &keyId : keyIds) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", encryption },
					{ u":keyId", keyId },
				}
			);
			execQuery(query);
		}
	});
}

auto TrustDb::removeKeys(const QString &encryption, const QString &keyOwnerJid) -> QXmppTask<void>
{
	return runTask([this, encryption, keyOwnerJid] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption AND ownerJid = :keyOwnerJid
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
				{ u":keyOwnerJid", keyOwnerJid },
			}
		);
	});
}

auto TrustDb::removeKeys(const QString &encryption) -> QXmppTask<void>
{
	return runTask([this, encryption] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);
	});
}

auto TrustDb::hasKey(const QString &encryption, const QString &keyOwnerJid, TrustLevels trustLevels)
	-> QXmppTask<bool>
{
	Q_ASSERT(int(trustLevels) > 0);
	return runTask([this, encryption, keyOwnerJid, trustLevels] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT COUNT(*)
				FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption AND ownerJid = :keyOwnerJid AND trustLevel IN (%1)
			)").arg(trustFlagsToString(trustLevels)),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
				{ u":keyOwnerJid", keyOwnerJid },
			}
		);

		if (query.next()) {
			return query.value(0).toInt() > 0;
		}
		return false;
	});
}

auto TrustDb::trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
	-> QXmppTask<TrustLevel>
{
	return runTask([this, encryption, keyOwnerJid, keyId] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT trustLevel
				FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption AND ownerJid = :keyOwnerJid AND keyId = :keyId
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
				{ u":keyOwnerJid", keyOwnerJid },
				{ u":keyId", keyId },
			}
		);

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
	TrustLevel trustLevel) -> QXmppTask<TrustChanges>
{
	return runTask([this, encryption, keyIds, trustLevel, account = m_accountJid] {
		auto query = createQuery();
		enum { RowId, TrustLevel_ };
		prepareQuery(query,
			QStringLiteral("SELECT rowid, trustLevel FROM trustKeys "
			"WHERE account = :account AND encryption = :encryption AND "
			"ownerJid = :jid AND keyId = :keyId"));

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
					{u":keyId", keyId},
				});
			execQuery(query);

			if (query.next()) {
				if (query.value(TrustLevel_).value<TrustLevel>() != trustLevel) {
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
	TrustLevel newTrustLevel) -> QXmppTask<TrustChanges>
{
	return runTask([this, encryption, keyOwnerJids, oldTrustLevel, newTrustLevel, account = m_accountJid] {
		TrustChanges result;
		auto &changes = result[encryption];
		std::vector<qint64> updateRowIds;

		enum { RowId, KeyId };
		auto query = createQuery();
		prepareQuery(query,
			QStringLiteral("SELECT rowid, keyId FROM trustKeys "
			"WHERE account = :account AND encryption = :encryption AND ownerJid = "
			":jid "
			"AND trustLevel = :old_trust"));

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
	const QList<QXmppTrustMessageKeyOwner> &keyOwners) -> QXmppTask<void>
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

	return runTask([this, keys = std::move(keys)] {
		transaction();
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO trustKeysUnprocessed (
					account,
					encryption,
					keyId,
					ownerJid,
					senderKeyId,
					trust
				)
				VALUES (
					:accountJid,
					:encryption,
					:keyId,
					:ownerJid,
					:senderKeyId,
					:trust
				)
			)")
		);
		for (const auto &key : keys) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", key.encryption },
					{ u":keyId", key.keyId },
					{ u":ownerJid", key.ownerJid },
					{ u":senderKeyId", key.senderKeyId },
					{ u":trust", key.trust },
				}
			);
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption,
	const QList<QByteArray> &keyIdsForAuthentication,
	const QList<QByteArray> &keyIdsForDistrusting) -> QXmppTask<void>
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

	return runTask([this, selectors = std::move(selectors)] {
		transaction();
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeysUnprocessed
				WHERE account = :accountJid AND encryption = :encryption AND keyId = :keyId AND trust = :trust
			)")
		);
		for (const auto &selector : selectors) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", selector.encryption },
					{ u":keyId", selector.keyId },
					{ u":trust", selector.trust },
				}
			);
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption,
	const QList<QByteArray> &senderKeyIds) -> QXmppTask<void>
{
	return runTask([this, encryption, senderKeyIds] {
		transaction();
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeysUnprocessed
				WHERE account = :accountJid AND encryption = :encryption AND senderKeyId = :keyId
			)")
		);
		for (const auto &keyId : senderKeyIds) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", encryption },
					{ u":keyId", keyId },
				}
			);
			execQuery(query);
		}
		commit();
	});
}

auto TrustDb::removeKeysForPostponedTrustDecisions(const QString &encryption) -> QXmppTask<void>
{
	return runTask([this, encryption] {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeysUnprocessed
				WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);
	});
}

auto TrustDb::keysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &senderKeyIds)
	-> QXmppTask<QHash<bool, QMultiHash<QString, QByteArray>>>
{
	if (senderKeyIds.isEmpty()) {
		return runTask([this, encryption] {
			QHash<bool, QMultiHash<QString, QByteArray>> result;

			auto query = createQuery();
			execQuery(
				query,
				QStringLiteral(R"(
					SELECT keyId, ownerJid, trust
					FROM trustKeysUnprocessed
					WHERE account = :accountJid AND encryption = :encryption
				)"),
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", encryption },
				}
			);

			enum { KeyId, OwnerJid, Trust };
			while (query.next()) {
				result[query.value(Trust).toBool()].insert(query.value(OwnerJid).toString(),
					query.value(KeyId).toByteArray());
			}

			return result;
		});
	}
	return runTask([this, encryption, senderKeyIds] {
		enum { KeyId, OwnerJid, Trust };
		QHash<bool, QMultiHash<QString, QByteArray>> result;

		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				SELECT keyId, ownerJid, trust
				FROM trustKeysUnprocessed
				WHERE account = :accountJid AND encryption = :encryption AND senderKeyId = :senderKeyId
			)")
		);

		for (const auto &senderKeyId : senderKeyIds) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", encryption },
					{ u":senderKeyId", senderKeyId },
				}
			);
			execQuery(query);
			while (query.next()) {
				result[query.value(Trust).toBool()].insert(query.value(OwnerJid).toString(),
					query.value(KeyId).toByteArray());
			}
		}

		return result;
	});
}

auto TrustDb::resetAll(const QString &encryption) -> QXmppTask<void>
{
	return runTask([this, encryption] {
		_resetSecurityPolicy(encryption);
		_resetOwnKey(encryption);

		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeys
				WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM trustKeysUnprocessed WHERE account = :accountJid AND encryption = :encryption
			)"),
			{
				{ u":accountJid", m_accountJid },
				{ u":encryption", encryption },
			}
		);
	});
}

auto TrustDb::resetAll() -> QXmppTask<void>
{
	return runTask([this] {
		auto query = createQuery();
		for (const auto &table : TRUST_DB_TABLES) {
			execQuery(
				query,
				QStringLiteral(R"(
					DELETE FROM %1
					WHERE account = :accountJid
				)").arg(QString::fromUtf8(table)),
				{
					{ u":accountJid", m_accountJid },
				}
			);
		}
	});
}

auto TrustDb::insertKeys(std::vector<Key> &&keys) -> QXmppTask<void>
{
	return runTask([this, keys = std::move(keys)] {
		transaction();
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO trustKeys (
					account,
					encryption,
					keyId,
					ownerJid,
					trustLevel
				)
				VALUES (
					:accountJid,
					:encryption,
					:keyId,
					:ownerJid,
					:trustLevel
				)
			)")
		);

		for (const auto &key : keys) {
			bindValues(
				query,
				{
					{ u":accountJid", m_accountJid },
					{ u":encryption", key.encryption },
					{ u":keyId", key.keyId },
					{ u":ownerJid", key.ownerJid },
					{ u":trustLevel", int(key.trustLevel) },
				}
			);
			execQuery(query);
		}
		commit();
	});
}

void TrustDb::_resetSecurityPolicy(const QString &encryption)
{
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			DELETE FROM trustSecurityPolicies
			WHERE account = :accountJid AND encryption = :encryption
		)"),
		{
			{ u":accountJid", m_accountJid },
			{ u":encryption", encryption },
		}
	);
}

void TrustDb::_resetOwnKey(const QString &encryption)
{
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			DELETE FROM trustOwnKeys
			WHERE account = :accountJid AND encryption = :encryption
		)"),
		{
			{ u":accountJid", m_accountJid },
			{ u":encryption", encryption },
		}
	);
}

void TrustDb::_setTrustLevel(TrustLevel trustLevel, qint64 rowId)
{
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			UPDATE trustKeys
			SET trustLevel = :trustLevel
			WHERE rowid = :rowId
		)"),
		{
			{ u":trustLevel", int(trustLevel) },
			{ u":rowId", rowId },
		}
	);
	Q_ASSERT(query.numRowsAffected() == 1);
	query.finish();
}
