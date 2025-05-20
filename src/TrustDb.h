// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppAtmTrustStorage.h>
#include <QXmppTrustLevel.h>
// Kaidan
#include "DatabaseComponent.h"

class AccountSettings;
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

    TrustDb(AccountSettings *accountSettings, QObject *xmppContext, QObject *parent = nullptr);

    auto securityPolicy(const QString &encryption) -> QXmppTask<SecurityPolicy> override;
    auto setSecurityPolicy(const QString &encryption, SecurityPolicy securityPolicy) -> QXmppTask<void> override;
    auto resetSecurityPolicy(const QString &encryption) -> QXmppTask<void> override;

    auto ownKey(const QString &encryption) -> QXmppTask<QByteArray> override;
    auto setOwnKey(const QString &encryption, const QByteArray &keyId) -> QXmppTask<void> override;
    auto resetOwnKey(const QString &encryption) -> QXmppTask<void> override;

    auto keys(const QString &encryption, QXmpp::TrustLevels trustLevels = {}) -> QXmppTask<KeysByLevel> override;
    auto keys(const QString &encryption, const QList<QString> &keyOwnerJids, QXmpp::TrustLevels trustLevels = {}) -> QXmppTask<KeysByOwner> override;
    auto addKeys(const QString &encryption,
                 const QString &keyOwnerJid,
                 const QList<QByteArray> &keyIds,
                 QXmpp::TrustLevel trustLevel = QXmpp::TrustLevel::AutomaticallyDistrusted) -> QXmppTask<void> override;
    auto removeKeys(const QString &encryption, const QList<QByteArray> &keyIds) -> QXmppTask<void> override;
    auto removeKeys(const QString &encryption, const QString &keyOwnerJid) -> QXmppTask<void> override;
    auto removeKeys(const QString &encryption) -> QXmppTask<void> override;
    auto hasKey(const QString &encryption, const QString &keyOwnerJid, QXmpp::TrustLevels trustLevels) -> QXmppTask<bool> override;

    auto trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId) -> QXmppTask<QXmpp::TrustLevel> override;
    auto _trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId) -> QXmpp::TrustLevel;
    static auto _trustLevel(Database *database, const QString &accountJid, const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
        -> QXmpp::TrustLevel;

    auto setTrustLevel(const QString &encryption, const QMultiHash<QString, QByteArray> &keyIds, QXmpp::TrustLevel trustLevel)
        -> QXmppTask<TrustChanges> override;
    auto setTrustLevel(const QString &encryption, const QList<QString> &keyOwnerJids, QXmpp::TrustLevel oldTrustLevel, QXmpp::TrustLevel newTrustLevel)
        -> QXmppTask<TrustChanges> override;

    // ATM
    auto addKeysForPostponedTrustDecisions(const QString &encryption, const QByteArray &senderKeyId, const QList<QXmppTrustMessageKeyOwner> &keyOwners)
        -> QXmppTask<void> override;
    auto removeKeysForPostponedTrustDecisions(const QString &encryption,
                                              const QList<QByteArray> &keyIdsForAuthentication,
                                              const QList<QByteArray> &keyIdsForDistrusting) -> QXmppTask<void> override;
    auto removeKeysForPostponedTrustDecisions(const QString &encryption, const QList<QByteArray> &senderKeyIds) -> QXmppTask<void> override;
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

    inline QString accountJid() const;

    AccountSettings *const m_accountSettings;
    QObject *m_xmppContext;
};
