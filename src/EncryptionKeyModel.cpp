// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionKeyModel.h"

EncryptionKeyModel::EncryptionKeyModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

EncryptionKeyModel::~EncryptionKeyModel()
{
}

QHash<int, QByteArray> EncryptionKeyModel::roleNames() const
{
    static const QHash<int, QByteArray> roles = {
        {static_cast<int>(Role::Label), QByteArrayLiteral("label")},
        {static_cast<int>(Role::KeyId), QByteArrayLiteral("keyId")},
    };

    return roles;
}

EncryptionController *EncryptionKeyModel::encryptionController() const
{
    return m_encryptionController;
}

void EncryptionKeyModel::setEncryptionController(EncryptionController *newEncryptionController)
{
    if (m_encryptionController != newEncryptionController) {
        m_encryptionController = newEncryptionController;

        if (!m_accountJid.isEmpty()) {
            setUp();
        }
    }
}

const QString &EncryptionKeyModel::accountJid() const
{
    return m_accountJid;
}

void EncryptionKeyModel::setAccountJid(const QString &newAccountJid)
{
    if (m_accountJid != newAccountJid) {
        m_accountJid = newAccountJid;
        Q_EMIT accountJidChanged();

        if (m_encryptionController) {
            setUp();
        }
    }
}

#include "moc_EncryptionKeyModel.cpp"
