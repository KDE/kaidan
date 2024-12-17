// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoController.h"

#include <QStringBuilder>
#include <QTimer>

#include <QXmppOmemoManager.h>

#include "AccountManager.h"
#include "Algorithms.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "Kaidan.h"
#include "PresenceCache.h"
#include "RosterModel.h"
#include "SystemUtils.h"

using namespace std::chrono_literals;

// interval to enable session building for new devices
constexpr auto SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL = 500ms;

OmemoController::OmemoController(QObject *parent)
    : QObject(parent)
{
    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(),
            &QXmppOmemoManager::trustLevelsChanged,
            this,
            [](const QMultiHash<QString, QByteArray> &modifiedKeys) {
                Q_EMIT EncryptionController::instance() -> keysChanged(AccountManager::instance()->jid(), modifiedKeys.keys());
            });

    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(),
            &QXmppOmemoManager::deviceAdded,
            this,
            [](const QString &jid, uint32_t) {
                Q_EMIT EncryptionController::instance() -> devicesChanged(AccountManager::instance()->jid(), {jid});
            });

    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(),
            &QXmppOmemoManager::deviceChanged,
            this,
            [](const QString &jid, uint32_t) {
                Q_EMIT EncryptionController::instance() -> devicesChanged(AccountManager::instance()->jid(), {jid});
            });

    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(),
            &QXmppOmemoManager::deviceRemoved,
            this,
            [](const QString &jid, uint32_t) {
                Q_EMIT EncryptionController::instance() -> devicesChanged(AccountManager::instance()->jid(), {jid});
            });

    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(), &QXmppOmemoManager::devicesRemoved, this, [](const QString &jid) {
        Q_EMIT EncryptionController::instance() -> devicesChanged(AccountManager::instance()->jid(), {jid});
    });

    connect(Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>(), &QXmppOmemoManager::allDevicesRemoved, this, [] {
        Q_EMIT EncryptionController::instance() -> allDevicesChanged();
    });
}

QFuture<void> OmemoController::load()
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    if (m_isLoaded) {
        interface.reportFinished();
    } else {
        runOnThread(Kaidan::instance()->client(), [this, interface]() mutable {
            Kaidan::instance()
                ->client()
                ->xmppClient()
                ->findExtension<QXmppOmemoManager>()
                ->setSecurityPolicy(QXmpp::TrustSecurityPolicy::Toakafa)
                .then(Kaidan::instance()->client(), [this, interface]() mutable {
                    Kaidan::instance()
                        ->client()
                        ->xmppClient()
                        ->findExtension<QXmppOmemoManager>()
                        ->changeDeviceLabel(QStringLiteral(APPLICATION_DISPLAY_NAME) % QStringLiteral(" - ") % SystemUtils::productName())
                        .then(Kaidan::instance()->client(), [this, interface](bool) mutable {
                            Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->load().then(
                                this,
                                [this, interface](bool isLoaded) mutable {
                                    m_isLoaded = isLoaded;
                                    interface.reportFinished();
                                });
                        });
                });
        });
    }

    return interface.future();
}

QFuture<void> OmemoController::setUp()
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    if (m_isLoaded) {
        enableSessionBuildingForNewDevices();
    } else {
        callRemoteTask(
            Kaidan::instance()->client(),
            [this]() {
                return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->setUp(), this};
            },
            this,
            [this, interface](bool isSetUp) mutable {
                if (isSetUp) {
                    // Enabling the session building for new devices is delayed after all (or at least
                    // most) devices are automatically received from the servers.
                    // That way, the sessions for those devices, which are only new during this setup,
                    // are not built at once.
                    // Instead, only sessions for devices that are received after this setup are built
                    // when opening a chat (i.e., even before sending the first message).
                    // The default behavior would otherwise build sessions not before sending a message
                    // which leads to longer waiting times.
                    QTimer::singleShot(SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL, this, &OmemoController::enableSessionBuildingForNewDevices);
                } else {
                    Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("End-to-end encryption via OMEMO 2 could not be set up"));
                    interface.reportFinished();
                }
            });
    }

    return interface.future();
}

QFuture<void> OmemoController::initializeAccount(const QString &accountJid)
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);
    buildMissingSessions(interface, {accountJid});
    return interface.future();
}

QFuture<void> OmemoController::initializeChat(const QString &accountJid, const QList<QString> &jids)
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);
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
        if (RosterModel::instance()->isPresenceSubscribedByItem(accountJid, jid)) {
            if (PresenceCache::instance()->resourcesCount(jid) == 0) {
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

    await(joinVoidFutures(this, std::move(futures)), this, [this, interface, accountJid, jids = jids]() mutable {
        jids.append(accountJid);
        buildMissingSessions(interface, jids);
    });

    return interface.future();
}

QFuture<bool> OmemoController::hasUsableDevices(const QList<QString> &jids)
{
    QFutureInterface<bool> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jids]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->devices(jids), this};
        },
        this,
        [interface](QList<QXmppOmemoDevice> devices) mutable {
            for (const auto &device : std::as_const(devices)) {
                const auto trustLevel = device.trustLevel();

                if (TRUST_LEVEL_USABLE.testFlag(trustLevel)) {
                    reportFinishedResult(interface, true);
                    return;
                }
            }

            reportFinishedResult(interface, false);
        });

    return interface.future();
}

QFuture<void> OmemoController::requestDeviceLists(const QList<QString> &jids)
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jids]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->requestDeviceLists(jids), this};
        },
        this,
        [interface](auto) mutable {
            interface.reportFinished();
        });

    return interface.future();
}

QFuture<void> OmemoController::subscribeToDeviceLists(const QList<QString> &jids)
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jids]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->subscribeToDeviceLists(jids), this};
        },
        this,
        [interface](auto) mutable {
            interface.reportFinished();
        });

    return interface.future();
}

QFuture<void> OmemoController::unsubscribeFromDeviceLists()
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->unsubscribeFromDeviceLists(), this};
        },
        this,
        [interface](auto) mutable {
            interface.reportFinished();
        });

    return interface.future();
}

QFuture<void> OmemoController::reset()
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->resetOwnDevice(), this};
        },
        this,
        [this, interface](auto) mutable {
            m_isLoaded = false;
            interface.reportFinished();
        });

    return interface.future();
}

QFuture<void> OmemoController::resetLocally()
{
    QFutureInterface<void> interface(QFutureInterfaceBase::Started);

    callVoidRemoteTask(
        Kaidan::instance()->client(),
        [this]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->resetOwnDeviceLocally(), this};
        },
        this,
        [this, interface]() mutable {
            m_isLoaded = false;
            interface.reportFinished();
        });

    return interface.future();
}

QFuture<QString> OmemoController::ownKey(const QString &)
{
    QFutureInterface<QString> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->ownKey(), this};
        },
        this,
        [interface](QByteArray &&key) mutable {
            reportFinishedResult(interface, QString::fromUtf8(key.toHex()));
        });

    return interface.future();
}

QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> OmemoController::keys(const QString &, const QList<QString> &jids, QXmpp::TrustLevels trustLevels)
{
    QFutureInterface<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jids, trustLevels]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->keys(jids, trustLevels), this};
        },
        this,
        [interface](QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &&keys) mutable {
            QHash<QString, QHash<QString, QXmpp::TrustLevel>> processedKeys;

            for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr) {
                const auto &jid = itr.key();
                const auto &trustLevels = itr.value();

                for (auto trustLevelItr = trustLevels.cbegin(); trustLevelItr != trustLevels.cend(); ++trustLevelItr) {
                    processedKeys[jid].insert(QString::fromUtf8(trustLevelItr.key().toHex()), trustLevelItr.value());
                }
            }

            reportFinishedResult(interface, std::move(processedKeys));
        });

    return interface.future();
}

QFuture<EncryptionController::OwnDevice> OmemoController::ownDevice(const QString &)
{
    QFutureInterface<EncryptionController::OwnDevice> interface(QFutureInterfaceBase::Started);

    runOnThread(
        Kaidan::instance()->client(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->ownDevice();
        },
        this,
        [interface](QXmppOmemoOwnDevice &&ownDevice) mutable {
            reportFinishedResult(interface, {ownDevice.label(), QString::fromUtf8(ownDevice.keyId().toHex())});
        });

    return interface.future();
}

QFuture<QList<EncryptionController::Device>> OmemoController::devices(const QString &, const QList<QString> &jids)
{
    QFutureInterface<QList<EncryptionController::Device>> interface(QFutureInterfaceBase::Started);

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, jids]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->devices(jids), this};
        },
        this,
        [interface](QList<QXmppOmemoDevice> &&devices) mutable {
            const auto processedDevices = transform(std::move(devices), [](const QXmppOmemoDevice &device) {
                return EncryptionController::Device{device.jid(), device.label(), QString::fromUtf8(device.keyId().toHex()), device.trustLevel()};
            });

            reportFinishedResult(interface, QList(processedDevices.cbegin(), processedDevices.cend()));
        });

    return interface.future();
};

void OmemoController::removeContactDevices(const QString &jid)
{
    runOnThread(Kaidan::instance()->client(), [jid]() {
        Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->removeContactDevices(jid);
    });
}

void OmemoController::buildMissingSessions(QFutureInterface<void> &interface, const QList<QString> &jids)
{
    runOnThread(Kaidan::instance()->client(), [this, interface, jids]() mutable {
        Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->buildMissingSessions(jids).then(this, [interface]() mutable {
            interface.reportFinished();
        });
    });
}

void OmemoController::enableSessionBuildingForNewDevices()
{
    runOnThread(Kaidan::instance()->client(), []() {
        Kaidan::instance()->client()->xmppClient()->findExtension<QXmppOmemoManager>()->setNewDeviceAutoSessionBuildingEnabled(true);
    });
}

#include "moc_OmemoController.cpp"
