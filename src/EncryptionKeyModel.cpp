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

void EncryptionKeyModel::setEncryptionController(EncryptionController *encryptionController)
{
    if (m_encryptionController != encryptionController) {
        m_encryptionController = encryptionController;

        if (!m_accountJid.isEmpty()) {
            setUp();
        }
    }
}

QString EncryptionKeyModel::accountJid() const
{
    return m_accountJid;
}

void EncryptionKeyModel::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        Q_EMIT accountJidChanged();

        if (m_encryptionController) {
            setUp();
        }
    }
}

#include "moc_EncryptionKeyModel.cpp"
