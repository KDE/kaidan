// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AtmController.h"

// QXmpp
#include <QXmppAtmManager.h>
#include <QXmppClient.h>
#include <QXmppUri.h>
// Kaidan
#include "Globals.h"
#include "Kaidan.h"
#include "TrustDb.h"

AtmController::AtmController(QObject *parent)
    : QObject(parent)
    , m_manager(Kaidan::instance()->client()->atmManager())
{
}

AtmController::~AtmController() = default;

void AtmController::setAccountJid(const QString &accountJid)
{
    TrustDb::instance()->setAccountJid(accountJid);
}

void AtmController::makeTrustDecisionsByUri(const QXmppUri &uri)
{
    runOnThread(m_manager, [this, uri]() {
        const auto trustMessageQuery = std::any_cast<QXmpp::Uri::TrustMessage>(uri.query());
        m_manager->makeTrustDecisions(trustMessageQuery.encryption,
                                      uri.jid(),
                                      keyIdsFromHex(trustMessageQuery.trustKeyIds),
                                      keyIdsFromHex(trustMessageQuery.distrustKeyIds));
    });
}

void AtmController::makeTrustDecisions(const QString &jid, const QList<QString> &keyIdsForAuthentication, const QList<QString> &keyIdsForDistrusting)
{
    runOnThread(m_manager, [this, jid, keyIdsForAuthentication, keyIdsForDistrusting]() {
        m_manager->makeTrustDecisions(XMLNS_OMEMO_2, jid, keyIdsFromHex(keyIdsForAuthentication), keyIdsFromHex(keyIdsForDistrusting));
    });
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
