// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class EncryptionController;

class TrustMessageUriGenerator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(EncryptionController *encryptionController MEMBER m_encryptionController WRITE setEncryptionController)
    Q_PROPERTY(QString jid MEMBER m_jid WRITE setJid)
    Q_PROPERTY(QString uri READ uri NOTIFY uriChanged)

public:
    explicit TrustMessageUriGenerator(QObject *parent = nullptr);

    EncryptionController *encryptionController() const;
    void setEncryptionController(EncryptionController *encryptionController);

    QString jid() const;
    void setJid(const QString &jid);

    QString uri() const;
    Q_SIGNAL void uriChanged();

protected:
    void setKeys(const QList<QString> &authenticatedKeys, const QList<QString> &distrustedKeys);
    void setUp();

private:
    void handleKeysChanged(const QList<QString> &jids);
    virtual void updateKeys() = 0;

    EncryptionController *m_encryptionController = nullptr;
    QString m_jid;

    QList<QString> m_authenticatedKeys;
    QList<QString> m_distrustedKeys;
};
