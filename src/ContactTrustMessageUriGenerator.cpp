// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ContactTrustMessageUriGenerator.h"

#include "EncryptionController.h"
#include "FutureUtils.h"

ContactTrustMessageUriGenerator::ContactTrustMessageUriGenerator(QObject *parent)
    : TrustMessageUriGenerator(parent)
{
    connect(this, &ContactTrustMessageUriGenerator::jidChanged, this, &ContactTrustMessageUriGenerator::handleJidChanged);
}

void ContactTrustMessageUriGenerator::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;

        if (!jid().isEmpty()) {
            setUp();
        }
    }
}

void ContactTrustMessageUriGenerator::handleJidChanged()
{
    if (!m_accountJid.isEmpty()) {
        setUp();
    }
}

void ContactTrustMessageUriGenerator::setUp()
{
    connect(EncryptionController::instance(),
            &EncryptionController::keysChanged,
            this,
            &ContactTrustMessageUriGenerator::handleKeysChanged,
            Qt::UniqueConnection);
    updateKeys();
}

void ContactTrustMessageUriGenerator::handleKeysChanged(const QString &accountJid, const QList<QString> &jids)
{
    if (m_accountJid == accountJid && jids.contains(jid())) {
        updateKeys();
    }
}

void ContactTrustMessageUriGenerator::updateKeys()
{
    await(EncryptionController::instance()->keys(m_accountJid, {jid()}, QXmpp::TrustLevel::ManuallyDistrusted | QXmpp::TrustLevel::Authenticated),
          this,
          [this](QHash<QString, QHash<QString, QXmpp::TrustLevel>> &&keys) mutable {
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
