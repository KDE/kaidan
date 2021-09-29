// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QXmppTrustLevel.h>
//
#include "DatabaseComponent.h"
#include <QXmppAtmTrustStorage.h>

class Database;
struct Key;
struct UnprocessedKey;

/**
 * TrustDb manages the e2ee trust management data for only one user account (unlike MessageDb
 * and RosterDb currently).
 */
class TrustDb : public DatabaseComponent, public QXmppAtmTrustStorage
{
	Q_OBJECT
public:
	using KeysByLevel = QHash<QXmpp::TrustLevel, QMultiHash<QString, QByteArray>>;
	using KeysByOwner = QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>>;
	using TrustChanges = QHash<QString, QMultiHash<QString, QByteArray>>;
	using SecurityPolicy = QXmpp::TrustSecurityPolicy;

	explicit TrustDb(Database *database, QString accountJid, QObject *parent = nullptr);
	~TrustDb() override = default;

	// Not thread-safe (but this shouldn't be a problem if it's only used from one place)
	inline QString accountJid() const
	{
		return m_accountJid;
	}
	inline void setAccountJid(QString newAccountJid)
	{
		m_accountJid = std::move(newAccountJid);
	}

	auto securityPolicy(const QString &encryption) -> QFuture<SecurityPolicy> override;
	auto setSecurityPolicy(const QString &encryption, SecurityPolicy securityPolicy) -> QFuture<void> override;
	auto resetSecurityPolicy(const QString &encryption) -> QFuture<void> override;

	auto ownKey(const QString &encryption) -> QFuture<QByteArray> override;
	auto setOwnKey(const QString &encryption, const QByteArray &keyId) -> QFuture<void> override;
	auto resetOwnKey(const QString &encryption) -> QFuture<void> override;

	auto keys(const QString &encryption, QXmpp::TrustLevels trustLevels = {}) -> QFuture<KeysByLevel> override;
	auto keys(const QString &encryption,
		const QList<QString> &keyOwnerJids,
		QXmpp::TrustLevels trustLevels = {}) -> QFuture<KeysByOwner> override;
	auto addKeys(const QString &encryption,
		const QString &keyOwnerJid,
		const QList<QByteArray> &keyIds,
		QXmpp::TrustLevel trustLevel = QXmpp::TrustLevel::AutomaticallyDistrusted)
		-> QFuture<void> override;
	auto removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) -> QFuture<void> override;
	auto removeKeys(const QString &encryption, const QString &keyOwnerJid) -> QFuture<void> override;
	auto removeKeys(const QString &encryption) -> QFuture<void> override;
	auto hasKey(const QString &encryption, const QString &keyOwnerJid, QXmpp::TrustLevels trustLevels)
		-> QFuture<bool> override;

	auto trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
		-> QFuture<QXmpp::TrustLevel> override;
	auto setTrustLevel(const QString &encryption,
		const QMultiHash<QString, QByteArray> &keyIds,
		QXmpp::TrustLevel trustLevel) -> QFuture<TrustChanges> override;
	auto setTrustLevel(const QString &encryption,
		const QList<QString> &keyOwnerJids,
		QXmpp::TrustLevel oldTrustLevel,
		QXmpp::TrustLevel newTrustLevel) -> QFuture<TrustChanges> override;

	// ATM
	auto addKeysForPostponedTrustDecisions(const QString &encryption,
		const QByteArray &senderKeyId,
		const QList<QXmppTrustMessageKeyOwner> &keyOwners) -> QFuture<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &keyIdsForAuthentication,
		const QList<QByteArray> &keyIdsForDistrusting) -> QFuture<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &senderKeyIds) -> QFuture<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption) -> QFuture<void> override;
	QFuture<QHash<bool, QMultiHash<QString, QByteArray>>> keysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &senderKeyIds = {}) override;

	auto resetAll(const QString &encryption) -> QFuture<void> override;
	auto resetAll() -> QFuture<void>;

private:
	auto insertKeys(std::vector<Key> &&) -> QFuture<void>;
	void _resetSecurityPolicy(const QString &encryption);
	void _resetOwnKey(const QString &encryption);
	void _setTrustLevel(QXmpp::TrustLevel trustLevel, qint64 rowId);

	QString m_accountJid;
};
