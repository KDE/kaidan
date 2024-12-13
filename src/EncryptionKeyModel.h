// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>

class EncryptionKeyModel : public QAbstractListModel
{
    Q_OBJECT

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

    QString accountJid() const;
    void setAccountJid(const QString &accountJid);
    Q_SIGNAL void accountJidChanged();

protected:
    virtual void setUp() = 0;

    QList<Key> m_keys;

private:
    QString m_accountJid;
};
