// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ContactTrustMessageUriGenerator.h"

// Kaidan
#include "EncryptionController.h"

ContactTrustMessageUriGenerator::ContactTrustMessageUriGenerator(QObject *parent)
    : TrustMessageUriGenerator(parent)
{
}

void ContactTrustMessageUriGenerator::updateKeys()
{
    encryptionController()
        ->keys({jid()}, QXmpp::TrustLevel::ManuallyDistrusted | QXmpp::TrustLevel::Authenticated)
        .then(this, [this](QHash<QString, QHash<QString, QXmpp::TrustLevel>> &&keys) mutable {
            const auto keyIds = keys.value(jid());

            QList<QString> authenticatedKeys;
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
}

#include "moc_ContactTrustMessageUriGenerator.cpp"
