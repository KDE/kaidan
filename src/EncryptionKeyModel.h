// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// Kaidan
#include "EncryptionController.h"

class EncryptionKeyModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(EncryptionController *encryptionController MEMBER m_encryptionController WRITE setEncryptionController)
    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)

public:
    enum class Role {
        Label = Qt::DisplayRole,
        KeyId,
    };

    struct Key {
        QString deviceLabel;
        QString id;
    };

    explicit EncryptionKeyModel(QObject *parent = nullptr);
    ~EncryptionKeyModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    EncryptionController *encryptionController() const;
    void setEncryptionController(EncryptionController *encryptionController);

    QString accountJid() const;
    void setAccountJid(const QString &accountJid);
    Q_SIGNAL void accountJidChanged();

protected:
    virtual void setUp() = 0;

    QList<Key> m_keys;

private:
    EncryptionController *m_encryptionController = nullptr;
    QString m_accountJid;
};
