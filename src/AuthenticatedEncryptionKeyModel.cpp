// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AuthenticatedEncryptionKeyModel.h"

#include "Algorithms.h"
#include "EncryptionController.h"
#include "FutureUtils.h"

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

    if (row == 0) {
        key.deviceLabel = m_ownKey.deviceLabel;
        key.id = m_ownKey.id;
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
    connect(EncryptionController::instance(),
            &EncryptionController::ownDeviceChanged,
            this,
            &AuthenticatedEncryptionKeyModel::handleOwnDeviceChanged,
            Qt::UniqueConnection);
    updateOwnKey();

    connect(EncryptionController::instance(),
            &EncryptionController::keysChanged,
            this,
            &AuthenticatedEncryptionKeyModel::handleDevicesChanged,
            Qt::UniqueConnection);
    connect(EncryptionController::instance(),
            &EncryptionController::devicesChanged,
            this,
            &AuthenticatedEncryptionKeyModel::handleDevicesChanged,
            Qt::UniqueConnection);
    updateKeys();
}

void AuthenticatedEncryptionKeyModel::handleOwnDeviceChanged(const QString &accountJid)
{
    if (this->accountJid() == accountJid) {
        updateOwnKey();
    }
}

void AuthenticatedEncryptionKeyModel::handleDevicesChanged(const QString &accountJid, const QList<QString> &jids)
{
    if (this->accountJid() == accountJid && jids.contains(accountJid)) {
        updateKeys();
    }
}

void AuthenticatedEncryptionKeyModel::updateOwnKey()
{
    await(EncryptionController::instance()->ownDevice(accountJid()), this, [this](EncryptionController::OwnDevice &&ownDevice) {
        beginInsertRows(QModelIndex(), 0, 0);
        m_ownKey = {ownDevice.label + QStringLiteral(" · ") + tr("This device"), ownDevice.keyId};
        endInsertRows();
    });
}

void AuthenticatedEncryptionKeyModel::updateKeys()
{
    await(EncryptionController::instance()->keys(accountJid(), {accountJid()}, QXmpp::TrustLevel::Authenticated),
          this,
          [this](QHash<QString, QHash<QString, QXmpp::TrustLevel>> &&keys) {
              const auto keyIds = keys.value(accountJid()).keys();

              auto authenticatedKeys = transform(keyIds, [](const QString &keyId) {
                  return Key{{}, keyId};
              });

              await(EncryptionController::instance()->devices(accountJid(), {accountJid()}),
                    this,
                    [this, authenticatedKeys](QList<EncryptionController::Device> &&devices) mutable {
                        for (const auto &device : std::as_const(devices)) {
                            for (auto &authenticatedKey : authenticatedKeys) {
                                if (device.keyId == authenticatedKey.id) {
                                    authenticatedKey.deviceLabel = device.label;
                                }
                            }
                        }

                        beginResetModel();
                        m_keys = {authenticatedKeys.cbegin(), authenticatedKeys.cend()};
                        endResetModel();
                    });
          });
}
