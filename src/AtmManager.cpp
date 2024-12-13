// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AtmManager.h"

#include <QXmppAtmManager.h>
#include <QXmppClient.h>
#include <QXmppUri.h>

#include "Globals.h"
#include "TrustDb.h"

AtmManager::AtmManager(QXmppClient *client, Database *database, QObject *parent)
    : QObject(parent)
    , m_trustStorage(new TrustDb(database, this, {}, this))
    , m_manager(client->addNewExtension<QXmppAtmManager>(m_trustStorage.get()))
{
    connect(this,
            &AtmManager::makeTrustDecisionsRequested,
            this,
            [this](const QString &jid, const QList<QString> &keyIdsForAuthentication, const QList<QString> &keyIdsForDistrusting) {
                makeTrustDecisions(jid, keyIdsFromHex(keyIdsForAuthentication), keyIdsFromHex(keyIdsForDistrusting));
            });
}

AtmManager::~AtmManager() = default;

void AtmManager::setAccountJid(const QString &accountJid)
{
    m_trustStorage->setAccountJid(accountJid);
}

void AtmManager::makeTrustDecisionsByUri(const QXmppUri &uri)
{
    const auto trustMessageQuery = std::any_cast<QXmpp::Uri::TrustMessage>(uri.query());
    m_manager->makeTrustDecisions(trustMessageQuery.encryption,
                                  uri.jid(),
                                  keyIdsFromHex(trustMessageQuery.trustKeyIds),
                                  keyIdsFromHex(trustMessageQuery.distrustKeyIds));
}

void AtmManager::makeTrustDecisions(const QString &jid, const QList<QByteArray> &keyIdsForAuthentication, const QList<QByteArray> &keyIdsForDistrusting)
{
    m_manager->makeTrustDecisions(XMLNS_OMEMO_2, jid, keyIdsForAuthentication, keyIdsForDistrusting);
}

QList<QByteArray> AtmManager::keyIdsFromHex(const QList<QString> &keyIds)
{
    QList<QByteArray> byteArrayKeyIds;

    const auto addKeyIdFromHex = [&byteArrayKeyIds](const QString &keyId) {
        byteArrayKeyIds.append(QByteArray::fromHex(keyId.toUtf8()));
    };

    std::for_each(keyIds.cbegin(), keyIds.cend(), addKeyIdFromHex);

    return byteArrayKeyIds;
}
