// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QFuture>
#include <QHash>
#include <QList>
#include <QObject>

#include <QXmppTrustLevel.h>

class OmemoController;

constexpr auto TRUST_LEVEL_DISTRUSTED = QXmpp::TrustLevel::AutomaticallyDistrusted | QXmpp::TrustLevel::ManuallyDistrusted;
constexpr auto TRUST_LEVEL_USABLE = ~TRUST_LEVEL_DISTRUSTED;
constexpr auto TRUST_LEVEL_AUTHENTICATABLE = ~(QXmpp::TrustLevel::Authenticated | QXmpp::TrustLevel::Undecided);

class EncryptionController : public QObject
{
    Q_OBJECT

public:
    struct OwnDevice {
        QString label;
        QString keyId;

        bool operator==(const OwnDevice &other) const = default;
    };

    struct Device {
        QString jid;
        QString label;
        QString keyId;
        QXmpp::TrustLevel trustLevel;

        bool operator==(const Device &other) const = default;
    };

    static EncryptionController *instance();

    explicit EncryptionController(QObject *parent = nullptr);
    ~EncryptionController() override;

    QFuture<void> load();
    QFuture<void> setUp();

    Q_INVOKABLE void initializeAccountFromQml(const QString &accountJid);
    QFuture<void> initializeAccount(const QString &accountJid);
    QFuture<void> initializeChat(const QString &accountJid, const QList<QString> &contactJids);

    QFuture<QString> ownKey(const QString &accountJid);

    QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> keys(const QString &accountJid, const QList<QString> &jids, QXmpp::TrustLevels trustLevels = {});
    Q_SIGNAL void keysChanged(const QString &accountJid, const QList<QString> &jids);

    QFuture<EncryptionController::OwnDevice> ownDevice(const QString &accountJid);
    Q_SIGNAL void ownDeviceChanged(const QString &accountJid);

    QFuture<QList<EncryptionController::Device>> devices(const QString &accountJid, const QList<QString> &jids);
    Q_SIGNAL void devicesChanged(const QString &accountJid, const QList<QString> &jids);
    Q_SIGNAL void allDevicesChanged();

    QFuture<bool> hasUsableDevices(const QList<QString> &jids);
    void removeContactDevices(const QString &jid);

    QFuture<void> unload();
    QFuture<void> reset();
    QFuture<void> resetLocally();

private:
    static EncryptionController *s_instance;

    std::unique_ptr<OmemoController> m_omemoController;
};
