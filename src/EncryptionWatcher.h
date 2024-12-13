// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>

class EncryptionWatcher : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
    Q_PROPERTY(QList<QString> jids READ jids WRITE setJids NOTIFY jidsChanged)

    Q_PROPERTY(bool hasDistrustedDevices READ hasDistrustedDevices NOTIFY hasDistrustedDevicesChanged)
    Q_PROPERTY(bool hasUsableDevices READ hasUsableDevices NOTIFY hasUsableDevicesChanged)
    Q_PROPERTY(bool hasAuthenticatableDevices READ hasAuthenticatableDevices NOTIFY hasAuthenticatableDevicesChanged)
    Q_PROPERTY(bool hasAuthenticatableDistrustedDevices READ hasAuthenticatableDistrustedDevices NOTIFY hasAuthenticatableDistrustedDevicesChanged)

public:
    explicit EncryptionWatcher(QObject *parent = nullptr);

    QString accountJid() const;
    void setAccountJid(const QString &accountJid);
    Q_SIGNAL void accountJidChanged();

    QList<QString> jids() const;
    void setJids(const QList<QString> &jids);
    Q_SIGNAL void jidsChanged();

    bool hasDistrustedDevices() const;
    Q_SIGNAL void hasDistrustedDevicesChanged();

    bool hasUsableDevices() const;
    Q_SIGNAL void hasUsableDevicesChanged();

    bool hasAuthenticatableDevices() const;
    Q_SIGNAL void hasAuthenticatableDevicesChanged();

    bool hasAuthenticatableDistrustedDevices() const;
    Q_SIGNAL void hasAuthenticatableDistrustedDevicesChanged();

private:
    void setUp();
    void handleDevicesChanged(const QString &accountJid, QList<QString> jids);
    void update();

    QString m_accountJid;
    QList<QString> m_jids;

    bool m_hasDistrustedDevices;
    bool m_hasUsableDevices;
    bool m_hasAuthenticatableDevices;
    bool m_hasAuthenticatableDistrustedDevices;
};
