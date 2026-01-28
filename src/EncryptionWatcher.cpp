// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionWatcher.h"

// Kaidan
#include "AccountController.h"
#include "Algorithms.h"
#include "EncryptionController.h"

EncryptionWatcher::EncryptionWatcher(QObject *parent)
    : QObject(parent)
{
}

void EncryptionWatcher::setEncryptionController(EncryptionController *encryptionController)
{
    if (m_encryptionController != encryptionController) {
        m_encryptionController = encryptionController;
        setUp();
    }
}

QString EncryptionWatcher::accountJid() const
{
    return m_accountJid;
}

void EncryptionWatcher::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        m_encryptionController = AccountController::instance()->account(accountJid)->encryptionController();
        Q_EMIT accountJidChanged();
        setUp();
    }
}

QList<QString> EncryptionWatcher::jids() const
{
    return m_jids;
}

void EncryptionWatcher::setJids(const QList<QString> &jids)
{
    if (m_jids != jids) {
        m_jids = jids;
        Q_EMIT jidsChanged();
        setUp();
    }
}

bool EncryptionWatcher::hasDistrustedDevices() const
{
    return m_hasDistrustedDevices;
}

bool EncryptionWatcher::hasUsableDevices() const
{
    return m_hasUsableDevices;
}

bool EncryptionWatcher::hasAuthenticatableDevices() const
{
    return m_hasAuthenticatableDevices;
}

bool EncryptionWatcher::hasAuthenticatableDistrustedDevices() const
{
    return m_hasAuthenticatableDistrustedDevices;
}

void EncryptionWatcher::setUp()
{
    if (m_encryptionController && !m_accountJid.isEmpty() && !m_jids.isEmpty()) {
        connect(m_encryptionController, &EncryptionController::devicesChanged, this, &EncryptionWatcher::handleDevicesChanged, Qt::UniqueConnection);
        update();
    }
}

void EncryptionWatcher::handleDevicesChanged(QList<QString> jids)
{
    if (containCommonElement(m_jids, jids)) {
        update();
    }
}

void EncryptionWatcher::update()
{
    m_encryptionController->devices(m_jids).then([this](QList<EncryptionController::Device> &&devices) {
        const auto distrustedDevicesCount = std::ranges::count_if(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_DISTRUSTED.testFlag(device.trustLevel);
        });

        if (m_hasDistrustedDevices != (distrustedDevicesCount != 0)) {
            m_hasDistrustedDevices = distrustedDevicesCount;
            Q_EMIT hasDistrustedDevicesChanged();
        }

        const auto hasUsableDevices = std::ranges::any_of(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_USABLE.testFlag(device.trustLevel);
        });

        if (m_hasUsableDevices != hasUsableDevices) {
            m_hasUsableDevices = hasUsableDevices;
            Q_EMIT hasUsableDevicesChanged();
        }

        const auto authenticatableDevicesCount = std::ranges::count_if(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_AUTHENTICATABLE.testFlag(device.trustLevel);
        });

        if (m_hasAuthenticatableDevices != (authenticatableDevicesCount != 0)) {
            m_hasAuthenticatableDevices = authenticatableDevicesCount;
            Q_EMIT hasAuthenticatableDevicesChanged();
        }

        const auto hasAuthenticatableDistrustedDevices = authenticatableDevicesCount == distrustedDevicesCount;

        if (m_hasAuthenticatableDistrustedDevices != hasAuthenticatableDistrustedDevices) {
            m_hasAuthenticatableDistrustedDevices = hasAuthenticatableDistrustedDevices;
            Q_EMIT hasAuthenticatableDistrustedDevicesChanged();
        }
    });
}

#include "moc_EncryptionWatcher.cpp"
