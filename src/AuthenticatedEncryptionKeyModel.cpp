// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AuthenticatedEncryptionKeyModel.h"

// Kaidan
#include "Algorithms.h"
#include "EncryptionController.h"

AuthenticatedEncryptionKeyModel::AuthenticatedEncryptionKeyModel(QObject *parent)
    : EncryptionKeyModel(parent)
{
}

AuthenticatedEncryptionKeyModel::~AuthenticatedEncryptionKeyModel()
{
}

int AuthenticatedEncryptionKeyModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }

    return m_keys.size() + 1;
}

QVariant AuthenticatedEncryptionKeyModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (!hasIndex(row, index.column(), index.parent())) {
        return {};
    }

    Key key;

    if (m_ownKey && row == 0) {
        key.deviceLabel = m_ownKey->deviceLabel;
        key.id = m_ownKey->id;
    } else {
        key = m_keys.at(row - 1);
    }

    switch (static_cast<Role>(role)) {
    case Role::Label:
        return key.deviceLabel;
    case Role::KeyId:
        return key.id;
    }

    return {};
}

void AuthenticatedEncryptionKeyModel::setUp()
{
    connect(encryptionController(), &EncryptionController::ownDeviceChanged, this, &AuthenticatedEncryptionKeyModel::updateOwnKey, Qt::UniqueConnection);
    updateOwnKey();

    connect(encryptionController(), &EncryptionController::keysChanged, this, &AuthenticatedEncryptionKeyModel::handleDevicesChanged, Qt::UniqueConnection);
    connect(encryptionController(), &EncryptionController::devicesChanged, this, &AuthenticatedEncryptionKeyModel::handleDevicesChanged, Qt::UniqueConnection);
    updateKeys();
}

void AuthenticatedEncryptionKeyModel::handleDevicesChanged(const QList<QString> &jids)
{
    if (jids.contains(accountJid())) {
        updateKeys();
    }
}

void AuthenticatedEncryptionKeyModel::updateOwnKey()
{
    const auto ownDevice = encryptionController()->ownDevice();

    auto setOwnKey = [this, ownDevice]() {
        m_ownKey = {ownDevice.label + QStringLiteral(" · ") + tr("This device"), ownDevice.keyId};
    };

    if (m_ownKey) {
        setOwnKey();
        const auto modelIndex = index(0);
        dataChanged(modelIndex, modelIndex);
    } else {
        beginInsertRows(QModelIndex(), 0, 0);
        setOwnKey();
        endInsertRows();
    }
}

void AuthenticatedEncryptionKeyModel::updateKeys()
{
    encryptionController()->keys({accountJid()}, QXmpp::TrustLevel::Authenticated).then(this, [this](QHash<QString, QHash<QString, QXmpp::TrustLevel>> &&keys) {
        const auto keyIds = keys.value(accountJid()).keys();

        auto authenticatedKeys = transform(keyIds, [](const QString &keyId) {
            return Key{{}, keyId};
        });

        encryptionController()->devices({accountJid()}).then(this, [this, authenticatedKeys](QList<EncryptionController::Device> &&devices) mutable {
            for (const auto &device : std::as_const(devices)) {
                for (auto &authenticatedKey : authenticatedKeys) {
                    if (device.keyId == authenticatedKey.id) {
                        authenticatedKey.deviceLabel = device.label;
                    }
                }
            }

            auto setKeys = [this, authenticatedKeys]() {
                m_keys = authenticatedKeys;
            };

            if (m_ownKey) {
                if (m_keys.isEmpty()) {
                    beginInsertRows(QModelIndex(), 1, authenticatedKeys.size());
                    setKeys();
                    endInsertRows();
                } else {
                    setKeys();
                    dataChanged(index(1), index(m_keys.size()));
                }
            } else {
                if (m_keys.isEmpty()) {
                    beginInsertRows(QModelIndex(), 0, authenticatedKeys.size() - 1);
                    setKeys();
                    endInsertRows();
                } else {
                    setKeys();
                    dataChanged(index(0), index(m_keys.size() - 1));
                }
            }
        });
    });
}

#include "moc_AuthenticatedEncryptionKeyModel.cpp"
