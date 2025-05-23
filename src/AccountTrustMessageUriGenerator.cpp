// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountTrustMessageUriGenerator.h"

// Kaidan
#include "EncryptionController.h"

AccountTrustMessageUriGenerator::AccountTrustMessageUriGenerator(QObject *parent)
    : TrustMessageUriGenerator(parent)
{
    connect(this, &AccountTrustMessageUriGenerator::jidChanged, this, &AccountTrustMessageUriGenerator::setUp);
}

void AccountTrustMessageUriGenerator::setUp()
{
    connect(EncryptionController::instance(),
            &EncryptionController::keysChanged,
            this,
            &AccountTrustMessageUriGenerator::handleKeysChanged,
            Qt::UniqueConnection);
    updateKeys();
}

void AccountTrustMessageUriGenerator::handleKeysChanged(const QString &accountJid, const QList<QString> &jids)
{
    if (jid() == accountJid && jids.contains(jid())) {
        updateKeys();
    }
}

void AccountTrustMessageUriGenerator::updateKeys()
{
    EncryptionController::instance()->ownKey(jid()).then(this, [this](QString &&key) {
        QList<QString> authenticatedKeys = {key};

        EncryptionController::instance()
            ->keys(jid(), {jid()}, QXmpp::TrustLevel::ManuallyDistrusted | QXmpp::TrustLevel::Authenticated)
            .then(this, [this, authenticatedKeys](QHash<QString, QHash<QString, QXmpp::TrustLevel>> &&keys) mutable {
                const auto keyIds = keys.value(jid());
                QList<QString> distrustedKeys;

                for (auto itr = keyIds.cbegin(); itr != keyIds.cend(); ++itr) {
                    if (itr.value() == QXmpp::TrustLevel::Authenticated) {
                        authenticatedKeys.append(itr.key());
                    } else {
                        distrustedKeys.append(itr.key());
                    }
                }

                setKeys(authenticatedKeys, distrustedKeys);
            });
    });
}

#include "moc_AccountTrustMessageUriGenerator.cpp"
