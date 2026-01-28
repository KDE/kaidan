// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AuthenticatableEncryptionKeyModel.h"

// Kaidan
#include "Algorithms.h"
#include "EncryptionController.h"

AuthenticatableEncryptionKeyModel::AuthenticatableEncryptionKeyModel(QObject *parent)
    : EncryptionKeyModel(parent)
{
}

AuthenticatableEncryptionKeyModel::~AuthenticatableEncryptionKeyModel()
{
}

int AuthenticatableEncryptionKeyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_keys.size();
}

QVariant AuthenticatableEncryptionKeyModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (!hasIndex(row, index.column(), index.parent())) {
        return {};
    }

    Key key = m_keys.at(row);

    switch (static_cast<EncryptionKeyModel::Role>(role)) {
    case EncryptionKeyModel::Role::Label:
        return key.deviceLabel;
    case EncryptionKeyModel::Role::KeyId:
        return key.id;
    }

    return {};
}

QString AuthenticatableEncryptionKeyModel::chatJid() const
{
    return m_chatJid;
}

void AuthenticatableEncryptionKeyModel::setChatJid(const QString &chatJid)
{
    if (m_chatJid != chatJid) {
        m_chatJid = chatJid;
        Q_EMIT chatJidChanged();

        setUp();
    }
}

bool AuthenticatableEncryptionKeyModel::contains(const QString &keyId)
{
    return std::ranges::any_of(m_keys, [keyId](const Key &key) {
        return key.id == keyId;
    });
}

void AuthenticatableEncryptionKeyModel::setUp()
{
    // Avoid setup when EncryptionKeyModel::setAccountJid() or EncryptionKeyModel::setEncryptionController() are called before setChatJid() is called
    // once.
    if (!accountJid().isEmpty() && !m_chatJid.isEmpty() && encryptionController()) {
        connect(encryptionController(),
                &EncryptionController::devicesChanged,
                this,
                &AuthenticatableEncryptionKeyModel::handleDevicesChanged,
                Qt::UniqueConnection);
        updateKeys();
    }
}

void AuthenticatableEncryptionKeyModel::handleDevicesChanged(QList<QString> jids)
{
    if (jids.contains(m_chatJid)) {
        updateKeys();
    }
}

void AuthenticatableEncryptionKeyModel::updateKeys()
{
    encryptionController()->devices({m_chatJid}).then([this](QList<EncryptionController::Device> &&devices) {
        beginResetModel();

        m_keys = transformFilter<QList<Key>>(devices, [](const EncryptionController::Device &device) -> std::optional<Key> {
            if ((~(QXmpp::TrustLevel::Authenticated | QXmpp::TrustLevel::Undecided)).testFlag(device.trustLevel)) {
                return Key{device.label, device.keyId};
            }

            return {};
        });

        endResetModel();
    });
}

#include "moc_AuthenticatableEncryptionKeyModel.cpp"
