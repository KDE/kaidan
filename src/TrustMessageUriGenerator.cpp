// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "TrustMessageUriGenerator.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "EncryptionController.h"
#include "Globals.h"

TrustMessageUriGenerator::TrustMessageUriGenerator(QObject *parent)
    : QObject(parent)
{
}

EncryptionController *TrustMessageUriGenerator::encryptionController() const
{
    return m_encryptionController;
}

void TrustMessageUriGenerator::setEncryptionController(EncryptionController *encryptionController)
{
    if (m_encryptionController != encryptionController) {
        m_encryptionController = encryptionController;

        if (!m_jid.isEmpty()) {
            setUp();
        }
    }
}

QString TrustMessageUriGenerator::jid() const
{
    return m_jid;
}

void TrustMessageUriGenerator::setJid(const QString &jid)
{
    if (m_jid != jid) {
        m_jid = jid;

        if (m_encryptionController) {
            setUp();
        }
    }
}

QString TrustMessageUriGenerator::uri() const
{
    QXmppUri uri;
    uri.setJid(m_jid);

    // Create a Trust Message URI only if there are keys for it.
    if (!m_authenticatedKeys.isEmpty() || !m_distrustedKeys.isEmpty()) {
        QXmpp::Uri::TrustMessage trustMessageQuery = {
            .encryption = XMLNS_OMEMO_2,
            .trustKeyIds = m_authenticatedKeys,
            .distrustKeyIds = m_distrustedKeys,
        };

        uri.setQuery(std::move(trustMessageQuery));
    }

    return uri.toString();
}

void TrustMessageUriGenerator::setKeys(const QList<QString> &authenticatedKeys, const QList<QString> &distrustedKeys)
{
    bool changed = false;

    if (m_authenticatedKeys != authenticatedKeys) {
        m_authenticatedKeys = authenticatedKeys;
        changed = true;
    }

    if (m_distrustedKeys != distrustedKeys) {
        m_distrustedKeys = distrustedKeys;
        changed = true;
    }

    // If a contact has no keys, uriChanged() must be emitted as well for the initial update.
    if (changed || (m_authenticatedKeys.isEmpty() && m_distrustedKeys.isEmpty())) {
        Q_EMIT uriChanged();
    }
}

void TrustMessageUriGenerator::setUp()
{
    connect(m_encryptionController, &EncryptionController::keysChanged, this, &TrustMessageUriGenerator::handleKeysChanged, Qt::UniqueConnection);
    updateKeys();
}

void TrustMessageUriGenerator::handleKeysChanged(const QList<QString> &jids)
{
    if (jids.contains(jid())) {
        updateKeys();
    }
}

#include "moc_TrustMessageUriGenerator.cpp"
