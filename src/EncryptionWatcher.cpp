// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "EncryptionWatcher.h"

// Kaidan
#include "Algorithms.h"
#include "EncryptionController.h"

EncryptionWatcher::EncryptionWatcher(QObject *parent)
    : QObject(parent)
{
}

void EncryptionWatcher::setEncryptionController(EncryptionController *encryptionController)
{
    if (m_encryptionController != encryptionController) {
        if (m_encryptionController) {
            disconnect(m_encryptionController, &EncryptionController::devicesChanged, this, nullptr);
        }

        m_encryptionController = encryptionController;

        if (!m_encryptionController) {
            reset();
        } else if (!m_jids.isEmpty()) {
            setUp();
        }
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

        if (m_jids.isEmpty()) {
            reset();
        } else if (m_encryptionController) {
            setUp();
        }
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
    connect(m_encryptionController, &EncryptionController::devicesChanged, this, &EncryptionWatcher::handleDevicesChanged, Qt::UniqueConnection);
    update();
}

void EncryptionWatcher::handleDevicesChanged(QList<QString> jids)
{
    if (containCommonElement(m_jids, jids)) {
        update();
    }
}

void EncryptionWatcher::update()
{
    m_encryptionController->devices(m_jids).then(this, [this, jids = m_jids](QList<EncryptionController::Device> &&devices) {
        // Skip devices of old JIDs.
        if (jids != m_jids) {
            return;
        }

        const auto distrustedDevicesCount = std::ranges::count_if(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_DISTRUSTED.testFlag(device.trustLevel);
        });

        setHasDistrustedDevices(distrustedDevicesCount);

        setHasUsableDevices(std::ranges::any_of(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_USABLE.testFlag(device.trustLevel);
        }));

        const auto authenticatableDevicesCount = std::ranges::count_if(devices, [](const EncryptionController::Device &device) {
            return TRUST_LEVEL_AUTHENTICATABLE.testFlag(device.trustLevel);
        });

        setHasAuthenticatableDevices(authenticatableDevicesCount);
        setHasAuthenticatableDistrustedDevices(authenticatableDevicesCount == distrustedDevicesCount);
    });
}

void EncryptionWatcher::reset()
{
    setHasDistrustedDevices(false);
    setHasUsableDevices(false);
    setHasAuthenticatableDevices(false);
    setHasAuthenticatableDistrustedDevices(false);
}

void EncryptionWatcher::setHasDistrustedDevices(bool hasDistrustedDevices)
{
    if (m_hasDistrustedDevices != hasDistrustedDevices) {
        m_hasDistrustedDevices = hasDistrustedDevices;
        Q_EMIT hasDistrustedDevicesChanged();
    }
}

void EncryptionWatcher::setHasUsableDevices(bool hasUsableDevices)
{
    if (m_hasUsableDevices != hasUsableDevices) {
        m_hasUsableDevices = hasUsableDevices;
        Q_EMIT hasUsableDevicesChanged();
    }
}

void EncryptionWatcher::setHasAuthenticatableDevices(bool hasAuthenticatableDevices)
{
    if (m_hasAuthenticatableDevices != hasAuthenticatableDevices) {
        m_hasAuthenticatableDevices = hasAuthenticatableDevices;
        Q_EMIT hasAuthenticatableDevicesChanged();
    }
}

void EncryptionWatcher::setHasAuthenticatableDistrustedDevices(bool hasAuthenticatableDistrustedDevices)
{
    if (m_hasAuthenticatableDistrustedDevices != hasAuthenticatableDistrustedDevices) {
        m_hasAuthenticatableDistrustedDevices = hasAuthenticatableDistrustedDevices;
        Q_EMIT hasAuthenticatableDistrustedDevicesChanged();
    }
}

#include "moc_EncryptionWatcher.cpp"
