// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoController.h"

// Qt
#include <QTimer>
// QXmpp
#include <QXmppOmemoManager.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "MainController.h"
#include "PresenceCache.h"
#include "RosterModel.h"
#include "SystemUtils.h"

using namespace std::chrono_literals;

// interval to enable session building for new devices
constexpr auto SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL = 500ms;

OmemoController::OmemoController(AccountSettings *accountSettings,
                                 EncryptionController *encryptionController,
                                 PresenceCache *presenceCache,
                                 QXmppOmemoManager *manager,
                                 QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_presenceCache(presenceCache)
    , m_manager(manager)
{
    connect(m_manager, &QXmppOmemoManager::trustLevelsChanged, this, [encryptionController](const QMultiHash<QString, QByteArray> &modifiedKeys) {
        Q_EMIT encryptionController->keysChanged(modifiedKeys.keys());
    });

    connect(m_manager, &QXmppOmemoManager::deviceAdded, this, [encryptionController](const QString &jid, uint32_t) {
        Q_EMIT encryptionController->devicesChanged({jid});
    });

    connect(m_manager, &QXmppOmemoManager::deviceChanged, this, [encryptionController](const QString &jid, uint32_t) {
        Q_EMIT encryptionController->devicesChanged({jid});
    });

    connect(m_manager, &QXmppOmemoManager::deviceRemoved, this, [encryptionController](const QString &jid, uint32_t) {
        Q_EMIT encryptionController->devicesChanged({jid});
    });

    connect(m_manager, &QXmppOmemoManager::devicesRemoved, this, [encryptionController](const QString &jid) {
        Q_EMIT encryptionController->devicesChanged({jid});
    });

    connect(m_manager, &QXmppOmemoManager::allDevicesRemoved, this, [encryptionController] {
        Q_EMIT encryptionController->allDevicesChanged();
    });
}

QFuture<void> OmemoController::load()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    if (m_isLoaded) {
        promise->finish();
    } else {
        callRemoteTask(
            m_manager,
            [this]() {
                return std::pair{m_manager->load(), this};
            },
            this,
            [this, promise](bool isLoaded) mutable {
                m_isLoaded = isLoaded;
                promise->finish();
            });
    }

    return promise->future();
}

QFuture<void> OmemoController::setUp()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    if (m_isLoaded) {
        enableSessionBuildingForNewDevices();
        promise->finish();
    } else {
        runOnThread(m_manager, [this, promise]() {
            m_manager->setSecurityPolicy(QXmpp::TrustSecurityPolicy::Toakafa).then(m_manager, [this, promise]() mutable {
                m_manager->setUp(QStringLiteral(APPLICATION_DISPLAY_NAME) % QStringLiteral(" - ") % SystemUtils::productName())
                    .then(this, [this, promise](bool isSetUp) mutable {
                        if (isSetUp) {
                            m_isLoaded = true;

                            // Enabling the session building for new devices is delayed after all (or at least
                            // most) devices are automatically received from the servers.
                            // That way, the sessions for those devices, which are only new during this setup,
                            // are not built at once.
                            // Instead, only sessions for devices that are received after this setup are built
                            // when opening a chat (i.e., even before sending the first message).
                            // The default behavior would otherwise build sessions not before sending a message
                            // which leads to longer waiting times.
                            QTimer::singleShot(SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL,
                                               this,
                                               &OmemoController::enableSessionBuildingForNewDevices);
                        } else {
                            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("End-to-end encryption via OMEMO 2 could not be set up"));
                            promise->finish();
                        }
                    });
            });
        });
    }

    return promise->future();
}

QFuture<void> OmemoController::unload()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    if (m_isLoaded) {
        unsubscribeFromDeviceLists().then([this, promise]() mutable {
            m_isLoaded = false;
            promise->finish();
        });
    } else {
        promise->finish();
    }

    return promise->future();
}

void OmemoController::initializeAccount()
{
    buildMissingSessions({m_accountSettings->jid()});
}

void OmemoController::initializeChat(const QList<QString> &jids)
{
    QList<QFuture<void>> futures;

    QList<QString> deviceListRequestJids;
    QList<QString> deviceListSubscriptionJids;

    for (const auto &jid : jids) {
        // Make it possible to use OMEMO encryption with the chat partner even if the chat partner has
        // no presence subscription or is offline.
        //
        // Subscribe to the OMEMO device list of the current chat partner if it is not automatically
        // requested via PEP's presence-based subscription ("auto-subscribe").
        // If there is a subscription but the chat partner is offline, the device list is requested
        // manually because it could result in the server not distributing the device list via PEP's
        // presence-based subscription.
        if (RosterModel::instance()->isPresenceSubscribedByItem(m_accountSettings->jid(), jid)) {
            if (m_presenceCache->resourcesCount(jid) == 0) {
                deviceListRequestJids.append(jid);
            }
        } else {
            deviceListSubscriptionJids.append(jid);
        }
    }

    if (!deviceListRequestJids.isEmpty()) {
        futures.append(requestDeviceLists(deviceListRequestJids));
    }

    if (!deviceListSubscriptionJids.isEmpty()) {
        futures.append(subscribeToDeviceLists(deviceListSubscriptionJids));
    }

    joinVoidFutures(this, std::move(futures)).then(this, [this, jids = jids]() mutable {
        jids.append(m_accountSettings->jid());
        buildMissingSessions(jids);
    });
}

QFuture<bool> OmemoController::hasUsableDevices(const QList<QString> &jids)
{
    auto promise = std::make_shared<QPromise<bool>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this, jids]() {
            return std::pair{m_manager->devices(jids), this};
        },
        this,
        [promise](QList<QXmppOmemoDevice> devices) mutable {
            for (const auto &device : std::as_const(devices)) {
                const auto trustLevel = device.trustLevel();

                if (TRUST_LEVEL_USABLE.testFlag(trustLevel)) {
                    reportFinishedResult(*promise, true);
                    return;
                }
            }

            reportFinishedResult(*promise, false);
        });

    return promise->future();
}

QFuture<void> OmemoController::requestDeviceLists(const QList<QString> &jids)
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this, jids]() {
            return std::pair{m_manager->requestDeviceLists(jids), this};
        },
        this,
        [promise](auto) mutable {
            promise->finish();
        });

    return promise->future();
}

QFuture<void> OmemoController::subscribeToDeviceLists(const QList<QString> &jids)
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this, jids]() {
            return std::pair{m_manager->subscribeToDeviceLists(jids), this};
        },
        this,
        [promise](auto) mutable {
            promise->finish();
        });

    return promise->future();
}

QFuture<void> OmemoController::reset()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this]() {
            return std::pair{m_manager->resetOwnDevice(), this};
        },
        this,
        [this, promise](auto) mutable {
            m_isLoaded = false;
            promise->finish();
        });

    return promise->future();
}

QFuture<void> OmemoController::resetLocally()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    callVoidRemoteTask(
        m_manager,
        [this]() {
            return std::pair{m_manager->resetOwnDeviceLocally(), this};
        },
        this,
        [this, promise]() mutable {
            m_isLoaded = false;
            promise->finish();
        });

    return promise->future();
}

QFuture<QString> OmemoController::ownKey()
{
    auto promise = std::make_shared<QPromise<QString>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this]() {
            return std::pair{m_manager->ownKey(), this};
        },
        this,
        [promise](QByteArray &&key) mutable {
            reportFinishedResult(*promise, QString::fromUtf8(key.toHex()));
        });

    return promise->future();
}

QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> OmemoController::keys(const QList<QString> &jids, QXmpp::TrustLevels trustLevels)
{
    auto promise = std::make_shared<QPromise<QHash<QString, QHash<QString, QXmpp::TrustLevel>>>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this, jids, trustLevels]() {
            return std::pair{m_manager->keys(jids, trustLevels), this};
        },
        this,
        [promise](QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &&keys) mutable {
            QHash<QString, QHash<QString, QXmpp::TrustLevel>> processedKeys;

            for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr) {
                const auto &jid = itr.key();
                const auto &trustLevels = itr.value();

                for (auto trustLevelItr = trustLevels.cbegin(); trustLevelItr != trustLevels.cend(); ++trustLevelItr) {
                    processedKeys[jid].insert(QString::fromUtf8(trustLevelItr.key().toHex()), trustLevelItr.value());
                }
            }

            reportFinishedResult(*promise, std::move(processedKeys));
        });

    return promise->future();
}

QFuture<EncryptionController::OwnDevice> OmemoController::ownDevice()
{
    auto promise = std::make_shared<QPromise<EncryptionController::OwnDevice>>();
    promise->start();

    runOnThread(
        m_manager,
        [this]() {
            return m_manager->ownDevice();
        },
        this,
        [promise](QXmppOmemoOwnDevice &&ownDevice) mutable {
            reportFinishedResult(*promise, {ownDevice.label(), QString::fromUtf8(ownDevice.keyId().toHex())});
        });

    return promise->future();
}

QFuture<QList<EncryptionController::Device>> OmemoController::devices(const QList<QString> &jids)
{
    auto promise = std::make_shared<QPromise<QList<EncryptionController::Device>>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this, jids]() {
            return std::pair{m_manager->devices(jids), this};
        },
        this,
        [promise](QList<QXmppOmemoDevice> &&devices) mutable {
            const auto processedDevices = transform(std::move(devices), [](const QXmppOmemoDevice &device) {
                return EncryptionController::Device{device.jid(), device.label(), QString::fromUtf8(device.keyId().toHex()), device.trustLevel()};
            });

            reportFinishedResult(*promise, processedDevices);
        });

    return promise->future();
};

void OmemoController::removeContactDevices(const QString &jid)
{
    runOnThread(m_manager, [this, jid]() {
        m_manager->removeContactDevices(jid);
    });
}

void OmemoController::buildMissingSessions(const QList<QString> &jids)
{
    runOnThread(m_manager, [this, jids]() mutable {
        m_manager->buildMissingSessions(jids);
    });
}

void OmemoController::enableSessionBuildingForNewDevices()
{
    runOnThread(m_manager, [this]() {
        m_manager->setNewDeviceAutoSessionBuildingEnabled(true);
    });
}

QFuture<void> OmemoController::unsubscribeFromDeviceLists()
{
    auto promise = std::make_shared<QPromise<void>>();
    promise->start();

    callRemoteTask(
        m_manager,
        [this]() {
            return std::pair{m_manager->unsubscribeFromDeviceLists(), this};
        },
        this,
        [promise](auto) mutable {
            promise->finish();
        });

    return promise->future();
}

#include "moc_OmemoController.cpp"
