// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class TrustMessageUriGenerator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString jid MEMBER m_jid WRITE setJid)
    Q_PROPERTY(QString uri READ uri NOTIFY uriChanged)

public:
    explicit TrustMessageUriGenerator(QObject *parent = nullptr);

    void setJid(const QString &jid);

    QString uri() const;
    Q_SIGNAL void uriChanged();

protected:
    QString jid() const;
    Q_SIGNAL void jidChanged();

    void setKeys(const QList<QString> &authenticatedKeys, const QList<QString> &distrustedKeys);

private:
    void setUp();
    void handleKeysChanged(const QString &accountJid, const QList<QString> &jids);

    QString m_jid;

    QList<QString> m_authenticatedKeys;
    QList<QString> m_distrustedKeys;
};
