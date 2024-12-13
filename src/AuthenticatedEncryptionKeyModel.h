// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "EncryptionKeyModel.h"

class AuthenticatedEncryptionKeyModel : public EncryptionKeyModel
{
    Q_OBJECT

public:
    explicit AuthenticatedEncryptionKeyModel(QObject *parent = nullptr);
    ~AuthenticatedEncryptionKeyModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

protected:
    void setUp() override;

private:
    void handleOwnDeviceChanged(const QString &accountJid);
    void handleDevicesChanged(const QString &accountJid, const QList<QString> &jids);

    void updateOwnKey();
    void updateKeys();

    Key m_ownKey;
};
