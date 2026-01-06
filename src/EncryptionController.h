// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QHash>
#include <QList>
#include <QObject>
// QXmpp
#include <QXmppTrustLevel.h>

class AccountSettings;
class OmemoController;
class PresenceCache;
class QXmppOmemoManager;

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

    EncryptionController(AccountSettings *accountSettings, PresenceCache *presenceCache, QXmppOmemoManager *omemoManager, QObject *parent = nullptr);

    QFuture<void> load();
    QFuture<void> setUp();

    Q_INVOKABLE void initializeAccount();
    void initializeChat(const QList<QString> &contactJids);

    QFuture<QString> ownKey();

    QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> keys(const QList<QString> &jids, QXmpp::TrustLevels trustLevels = {});
    Q_SIGNAL void keysChanged(const QList<QString> &jids);

    OwnDevice ownDevice();
    Q_SIGNAL void ownDeviceChanged();

    QFuture<QList<EncryptionController::Device>> devices(const QList<QString> &jids);
    Q_SIGNAL void devicesChanged(const QList<QString> &jids);
    Q_SIGNAL void allDevicesChanged();

    QFuture<bool> hasUsableDevices(const QList<QString> &jids);
    void removeContactDevices(const QString &jid);

    QFuture<void> unload();
    QFuture<void> reset();
    QFuture<void> resetLocally();

private:
    OmemoController *const m_omemoController;
};
