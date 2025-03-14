// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionWatcher.h"

// Kaidan
#include "Algorithms.h"
#include "EncryptionController.h"
#include "FutureUtils.h"

EncryptionWatcher::EncryptionWatcher(QObject *parent)
    : QObject(parent)
{
}

QString EncryptionWatcher::accountJid() const
{
    return m_accountJid;
}

void EncryptionWatcher::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
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
    if (!m_accountJid.isEmpty() && !m_jids.isEmpty()) {
        connect(EncryptionController::instance(), &EncryptionController::devicesChanged, this, &EncryptionWatcher::handleDevicesChanged, Qt::UniqueConnection);
        update();
    }
}

void EncryptionWatcher::handleDevicesChanged(const QString &, QList<QString> jids)
{
    if (containCommonElement(m_jids, jids)) {
        update();
    }
}

void EncryptionWatcher::update()
{
    await(EncryptionController::instance()->devices(m_accountJid, m_jids), this, [this](QList<EncryptionController::Device> &&devices) {
        const auto distrustedDevicesCount = std::count_if(devices.cbegin(), devices.cend(), [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_DISTRUSTED.testFlag(device.trustLevel);
        });

        if (m_hasDistrustedDevices != (distrustedDevicesCount != 0)) {
            m_hasDistrustedDevices = distrustedDevicesCount;
            Q_EMIT hasDistrustedDevicesChanged();
        }

        const auto hasUsableDevices = std::any_of(devices.cbegin(), devices.cend(), [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_USABLE.testFlag(device.trustLevel);
        });

        if (m_hasUsableDevices != hasUsableDevices) {
            m_hasUsableDevices = hasUsableDevices;
            Q_EMIT hasUsableDevicesChanged();
        }

        const auto authenticatableDevicesCount = std::count_if(devices.cbegin(), devices.cend(), [](const EncryptionController::Device &device) {
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
