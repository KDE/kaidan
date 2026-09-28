// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QtQml/qqmlregistration.h>

class EncryptionController;

class EncryptionWatcher : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(EncryptionController *encryptionController MEMBER m_encryptionController WRITE setEncryptionController)
    Q_PROPERTY(QList<QString> jids READ jids WRITE setJids NOTIFY jidsChanged)

    Q_PROPERTY(bool hasDistrustedDevices READ hasDistrustedDevices NOTIFY hasDistrustedDevicesChanged)
    Q_PROPERTY(bool hasUsableDevices READ hasUsableDevices NOTIFY hasUsableDevicesChanged)
    Q_PROPERTY(bool hasAuthenticatableDevices READ hasAuthenticatableDevices NOTIFY hasAuthenticatableDevicesChanged)
    Q_PROPERTY(bool hasAuthenticatableDistrustedDevices READ hasAuthenticatableDistrustedDevices NOTIFY hasAuthenticatableDistrustedDevicesChanged)

public:
    explicit EncryptionWatcher(QObject *parent = nullptr);

    void setEncryptionController(EncryptionController *encryptionController);

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
    void handleDevicesChanged(QList<QString> jids);
    void update();
    void reset();

    void setHasDistrustedDevices(bool hasDistrustedDevices);
    void setHasUsableDevices(bool hasUsableDevices);
    void setHasAuthenticatableDevices(bool hasAuthenticatableDevices);
    void setHasAuthenticatableDistrustedDevices(bool hasAuthenticatableDistrustedDevices);

    EncryptionController *m_encryptionController = nullptr;
    QList<QString> m_jids;

    bool m_hasDistrustedDevices = false;
    bool m_hasUsableDevices = false;
    bool m_hasAuthenticatableDevices = false;
    bool m_hasAuthenticatableDistrustedDevices = false;
};
