// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionController.h"

// Kaidan
#include "OmemoController.h"

EncryptionController::EncryptionController(AccountSettings *accountSettings, PresenceCache *presenceCache, QXmppOmemoManager *omemoManager, QObject *parent)
    : QObject(parent)
    , m_omemoController(new OmemoController(accountSettings, this, presenceCache, omemoManager, this))
{
}

QFuture<void> EncryptionController::load()
{
    return m_omemoController->load();
}

QFuture<void> EncryptionController::setUp()
{
    return m_omemoController->setUp();
}

void EncryptionController::initializeAccount()
{
    m_omemoController->initializeAccount();
}

void EncryptionController::initializeChat(const QList<QString> &contactJids)
{
    m_omemoController->initializeChat(contactJids);
}

QFuture<QString> EncryptionController::ownKey()
{
    return m_omemoController->ownKey();
}

QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> EncryptionController::keys(const QList<QString> &jids, QXmpp::TrustLevels trustLevels)
{
    return m_omemoController->keys(jids, trustLevels);
}

EncryptionController::OwnDevice EncryptionController::ownDevice()
{
    return m_omemoController->ownDevice();
}

QFuture<QList<EncryptionController::Device>> EncryptionController::devices(const QList<QString> &jids)
{
    return m_omemoController->devices(jids);
}

QFuture<bool> EncryptionController::hasUsableDevices(const QList<QString> &jids)
{
    return m_omemoController->hasUsableDevices(jids);
}

void EncryptionController::removeContactDevices(const QString &jid)
{
    m_omemoController->removeContactDevices(jid);
}

QFuture<void> EncryptionController::unload()
{
    return m_omemoController->unload();
}

QFuture<void> EncryptionController::reset()
{
    return m_omemoController->reset();
}

QFuture<void> EncryptionController::resetLocally()
{
    return m_omemoController->resetLocally();
}

#include "moc_EncryptionController.cpp"
