// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AtmController.h"

// QXmpp
#include <QXmppAtmManager.h>
#include <QXmppUri.h>
// Kaidan
#include "CredentialsValidator.h"
#include "FutureUtils.h"
#include "Globals.h"

AtmController::AtmController(QXmppAtmManager *manager, QObject *parent)
    : QObject(parent)
    , m_manager(manager)
{
}

AtmController::~AtmController() = default;

AtmController::TrustDecisionWithUriResult AtmController::makeTrustDecisionsWithUri(const QString &uriString, const QString &expectedJid)
{
    if (const auto uriParsingResult = QXmppUri::fromString(uriString); std::holds_alternative<QXmppUri>(uriParsingResult)) {
        const auto uri = std::get<QXmppUri>(uriParsingResult);
        const auto jid = uri.jid();
        const auto query = uri.query();

        if (query.type() != typeid(QXmpp::Uri::TrustMessage) || !CredentialsValidator::isUserJidValid(jid)) {
            return TrustDecisionWithUriResult::InvalidUri;
        }

        if (expectedJid.isEmpty() || jid == expectedJid) {
            const auto trustMessageQuery = std::any_cast<QXmpp::Uri::TrustMessage>(query);

            if (trustMessageQuery.encryption != XMLNS_OMEMO_2 || (trustMessageQuery.trustKeyIds.isEmpty() && trustMessageQuery.distrustKeyIds.isEmpty())) {
                return TrustDecisionWithUriResult::InvalidUri;
            }

            makeTrustDecisions(uri.jid(), trustMessageQuery.trustKeyIds, trustMessageQuery.distrustKeyIds);
            return TrustDecisionWithUriResult::MakingTrustDecisions;
        }

        return TrustDecisionWithUriResult::JidUnexpected;
    }

    return TrustDecisionWithUriResult::InvalidUri;
}

void AtmController::makeTrustDecisions(const QString &jid, const QList<QString> &keyIdsForAuthentication, const QList<QString> &keyIdsForDistrusting)
{
    m_manager->makeTrustDecisions(XMLNS_OMEMO_2, jid, keyIdsFromHex(keyIdsForAuthentication), keyIdsFromHex(keyIdsForDistrusting));
}

QFuture<QXmpp::TrustLevel> AtmController::trustLevel(const QString &encryption, const QString &keyOwnerJid, const QByteArray &keyId)
{
    auto promise = std::make_shared<QPromise<QXmpp::TrustLevel>>();
    promise->start();

    m_manager->trustLevel(encryption, keyOwnerJid, keyId).then(this, [promise](QXmpp::TrustLevel &&trustLevel) mutable {
        reportFinishedResult(*promise, std::move(trustLevel));
    });

    return promise->future();
}

QList<QByteArray> AtmController::keyIdsFromHex(const QList<QString> &keyIds)
{
    QList<QByteArray> byteArrayKeyIds;

    const auto addKeyIdFromHex = [&byteArrayKeyIds](const QString &keyId) {
        byteArrayKeyIds.append(QByteArray::fromHex(keyId.toUtf8()));
    };

    std::for_each(keyIds.cbegin(), keyIds.cend(), addKeyIdFromHex);

    return byteArrayKeyIds;
}

#include "moc_AtmController.cpp"
