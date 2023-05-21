// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
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

	explicit TrustDb(Database *database, QObject *xmppContext, QString accountJid, QObject *parent = nullptr);
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

	auto securityPolicy(const QString &encryption) -> QXmppTask<SecurityPolicy> override;
	auto setSecurityPolicy(const QString &encryption, SecurityPolicy securityPolicy) -> QXmppTask<void> override;
	auto resetSecurityPolicy(const QString &encryption) -> QXmppTask<void> override;

	auto ownKey(const QString &encryption) -> QXmppTask<QByteArray> override;
	auto setOwnKey(const QString &encryption, const QByteArray &keyId) -> QXmppTask<void> override;
	auto resetOwnKey(const QString &encryption) -> QXmppTask<void> override;

	auto keys(const QString &encryption, QXmpp::TrustLevels trustLevels = {}) -> QXmppTask<KeysByLevel> override;
	auto keys(const QString &encryption,
		const QList<QString> &keyOwnerJids,
		QXmpp::TrustLevels trustLevels = {}) -> QXmppTask<KeysByOwner> override;
	auto addKeys(const QString &encryption,
		const QString &keyOwnerJid,
		const QList<QByteArray> &keyIds,
		QXmpp::TrustLevel trustLevel = QXmpp::TrustLevel::AutomaticallyDistrusted)
		-> QXmppTask<void> override;
	auto removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) -> QXmppTask<void> override;
	auto removeKeys(const QString &encryption, const QString &keyOwnerJid) -> QXmppTask<void> override;
	auto removeKeys(const QString &encryption) -> QXmppTask<void> override;
	auto hasKey(const QString &encryption, const QString &keyOwnerJid, QXmpp::TrustLevels trustLevels)
		-> QXmppTask<bool> override;

	auto trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
		-> QXmppTask<QXmpp::TrustLevel> override;
	auto setTrustLevel(const QString &encryption,
		const QMultiHash<QString, QByteArray> &keyIds,
		QXmpp::TrustLevel trustLevel) -> QXmppTask<TrustChanges> override;
	auto setTrustLevel(const QString &encryption,
		const QList<QString> &keyOwnerJids,
		QXmpp::TrustLevel oldTrustLevel,
		QXmpp::TrustLevel newTrustLevel) -> QXmppTask<TrustChanges> override;

	// ATM
	auto addKeysForPostponedTrustDecisions(const QString &encryption,
		const QByteArray &senderKeyId,
		const QList<QXmppTrustMessageKeyOwner> &keyOwners) -> QXmppTask<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &keyIdsForAuthentication,
		const QList<QByteArray> &keyIdsForDistrusting) -> QXmppTask<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &senderKeyIds) -> QXmppTask<void> override;
	auto removeKeysForPostponedTrustDecisions(const QString &encryption) -> QXmppTask<void> override;
	QXmppTask<QHash<bool, QMultiHash<QString, QByteArray>>> keysForPostponedTrustDecisions(const QString &encryption,
		const QList<QByteArray> &senderKeyIds = {}) override;

	auto resetAll(const QString &encryption) -> QXmppTask<void> override;
	auto resetAll() -> QXmppTask<void>;

private:
	template<typename Functor>
	auto runTask(Functor function) const
	{
		return runAsyncTask(m_xmppContext, dbWorker(), function);
	}

	auto insertKeys(std::vector<Key> &&) -> QXmppTask<void>;
	void _resetSecurityPolicy(const QString &encryption);
	void _resetOwnKey(const QString &encryption);
	void _setTrustLevel(QXmpp::TrustLevel trustLevel, qint64 rowId);

	QObject *m_xmppContext;
	QString m_accountJid;
};
