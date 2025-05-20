// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "EncryptionKeyModel.h"

class AuthenticatableEncryptionKeyModel : public EncryptionKeyModel
{
    Q_OBJECT

    Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)

public:
    explicit AuthenticatableEncryptionKeyModel(QObject *parent = nullptr);
    ~AuthenticatableEncryptionKeyModel() override;

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    QString chatJid() const;
    void setChatJid(const QString &chatJid);
    Q_SIGNAL void chatJidChanged();

    Q_INVOKABLE bool contains(const QString &keyId);

protected:
    virtual void setUp() override;

private:
    void handleDevicesChanged(QList<QString> jids);
    void updateKeys();

    QString m_chatJid;
};
