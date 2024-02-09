// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoManager.h"

#include <QStringBuilder>
#include <QTimer>

#include <QXmppOmemoManager.h>

#include "AccountManager.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "MessageModel.h"
#include "OmemoCache.h"
#include "OmemoDb.h"
#include "PresenceCache.h"
#include "RosterModel.h"
#include "SystemUtils.h"

using namespace std::chrono_literals;

// interval to enable session building for new devices
constexpr auto SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL = 500ms;

OmemoManager::OmemoManager(QXmppClient *client, Database *database, QObject *parent)
	: QObject(parent),
	  m_omemoStorage(new OmemoDb(database, this, {}, this)),
	  m_manager(client->addNewExtension<QXmppOmemoManager>(m_omemoStorage.get()))
{
	client->setEncryptionExtension(m_manager);

	connect(this, &OmemoManager::retrieveOwnKeyRequested, this, [this, client]() {
		m_lastRequestedKeyOwnerJids = { client->configuration().jidBare() };
		retrieveOwnKey();
	});

	connect(this, &OmemoManager::initializeChatRequested, this, [this](const QString &accountJid) {
		initializeChat(accountJid, accountJid);
	});

	connect(m_manager, &QXmppOmemoManager::trustLevelsChanged, this, [this](const QMultiHash<QString, QByteArray> &modifiedKeys) {
		retrieveKeysForRequestedJids(modifiedKeys.keys());
	});

	connect(m_manager, &QXmppOmemoManager::deviceAdded, this, [this](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::deviceChanged, this, [this](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::deviceRemoved, this, [this](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::devicesRemoved, this, &OmemoManager::retrieveDevicesForRequestedJids);

	connect(m_manager, &QXmppOmemoManager::allDevicesRemoved, this, [this] {
		for (const auto &jid : std::as_const(m_lastRequestedKeyOwnerJids)) {
			updateCachedDevices(jid, {}, {}, {}, {});
		}
	});
}

OmemoManager::~OmemoManager() = default;

void OmemoManager::setAccountJid(const QString &accountJid)
{
	m_omemoStorage->setAccountJid(accountJid);
}

QFuture<void> OmemoManager::load()
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	if (m_isLoaded) {
		interface.reportFinished();
	} else {
		auto future = m_manager->setSecurityPolicy(QXmpp::TrustSecurityPolicy::Toakafa);
		future.then(this, [this, interface]() mutable {
			auto future = m_manager->changeDeviceLabel(QStringLiteral(APPLICATION_DISPLAY_NAME) % QStringLiteral(" - ") % SystemUtils::productName());
			future.then(this, [this, interface](bool) mutable {
				auto future = m_manager->load();
				future.then(this, [this, interface](bool isLoaded) mutable {
					m_isLoaded = isLoaded;
					interface.reportFinished();
				});
			});
		});
	}

	return interface.future();
}

QFuture<void> OmemoManager::setUp()
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	if (m_isLoaded) {
		enableSessionBuildingForNewDevices();
	} else {
		auto future = m_manager->setUp();
		future.then(this, [this, interface](bool isSetUp) mutable {
			if (isSetUp) {
				// Enabling the session building for new devices is delayed after all (or at least
				// most) devices are automatically received from the servers.
				// That way, the sessions for those devices, which are only new during this setup,
				// are not built at once.
				// Instead, only sessions for devices that are received after this setup are built
				// when opening a chat (i.e., even before sending the first message).
				// The default behavior would otherwise build sessions not before sending a message
				// which leads to longer waiting times.
				QTimer::singleShot(SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL, this, &OmemoManager::enableSessionBuildingForNewDevices);

				// Retrieve the own key before opening the first chat.
				// It can be used when presenting the own QR code.
				auto future = retrieveOwnKey();
				await(future, this, [interface]() mutable {
					interface.reportFinished();
				});
			} else {
				Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("End-to-end encryption via OMEMO 2 could not be set up"));
				interface.reportFinished();
			}
		});
	}

	return interface.future();
}

QFuture<void> OmemoManager::retrieveKeys(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->keys(jids, ~ QXmpp::TrustLevels { QXmpp::TrustLevel::Undecided });
	future.then(this, [this, interface](QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &&keys) mutable {
		auto future = retrieveOwnKey(std::move(keys));
		await(future, this, [interface]() mutable {
			interface.reportFinished();
		});
	});

	return interface.future();
}

QFuture<bool> OmemoManager::hasUsableDevices(const QList<QString> &jids)
{
	QFutureInterface<bool> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->devices(jids);
	future.then(this, [=](QVector<QXmppOmemoDevice> devices) mutable {
		for (const auto &device : std::as_const(devices)) {
			const auto trustLevel = device.trustLevel();

			if (!(QXmpp::TrustLevel::AutomaticallyDistrusted | QXmpp::TrustLevel::ManuallyDistrusted).testFlag(trustLevel)) {
				reportFinishedResult(interface, true);
				return;
			}
		}

		reportFinishedResult(interface, false);
	});

	return interface.future();
}

QFuture<void> OmemoManager::requestDeviceLists(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->requestDeviceLists(jids);
	future.then(this, [interface](auto &&) mutable {
		interface.reportFinished();
	});

	return interface.future();
}

QFuture<void> OmemoManager::subscribeToDeviceLists(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->subscribeToDeviceLists(jids);
	future.then(this, [interface](auto &&) mutable {
		interface.reportFinished();
	});

	return interface.future();
}

QFuture<void> OmemoManager::unsubscribeFromDeviceLists()
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->unsubscribeFromDeviceLists();
	future.then(this, [interface](auto &&) mutable {
		interface.reportFinished();
	});

	return interface.future();
}

QXmppTask<bool> OmemoManager::resetOwnDevice()
{
	return m_manager->resetOwnDevice();
}

void OmemoManager::enableSessionBuildingForNewDevices()
{
	m_manager->setNewDeviceAutoSessionBuildingEnabled(true);
}

QFuture<void> OmemoManager::initializeChat(const QString &accountJid, const QString &chatJid)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	const QList<QString> jids = { accountJid, chatJid };
	m_lastRequestedKeyOwnerJids = jids;

	auto initializeSessionsKeysAndDevices = [this, interface, jids]() mutable {
		auto future = m_manager->buildMissingSessions(jids);
		future.then(this, [this, interface, jids]() mutable {
			retrieveKeys(jids);
			retrieveDevices(jids);
			interface.reportFinished();
		});
	};

	// Make it possible to use OMEMO encryption with the chat partner even if the chat partner has
	// no presence subscription or is offline.
	if (accountJid == chatJid) {
		initializeSessionsKeysAndDevices();
	} else {
		// Subscribe to the OMEMO device list of the current chat partner if it is not automatically
		// requested via PEP's presence-based subscription ("auto-subscribe").
		// If there is a subscription but the chat partner is offline, the device list is requested
		// manually because it could result in the server not distributing the device list via PEP's
		// presence-based subscription.
		runOnThread(RosterModel::instance(), [accountJid, chatJid]() {
			return std::tuple {
				RosterModel::instance()->isPresenceSubscribedByItem(accountJid, chatJid),
				PresenceCache::instance()->resourcesCount(chatJid)
			};
		}, this, [=, this](std::tuple<bool, int> result) mutable {
			auto [isPresenceSubscribed, resourcesCount] = result;
			if (isPresenceSubscribed) {
				if (resourcesCount == 0) {
					auto future = requestDeviceLists({ chatJid });
					await(future, this, initializeSessionsKeysAndDevices);
				} else {
					initializeSessionsKeysAndDevices();
				}
			} else {
				auto future = subscribeToDeviceLists({ chatJid });
				await(future, this, initializeSessionsKeysAndDevices);
			}
		});
	}

	return interface.future();
}

void OmemoManager::removeContactDevices(const QString &jid)
{
	m_manager->removeContactDevices(jid);
}

QFuture<void> OmemoManager::retrieveOwnKey(QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> keys)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->ownKey();
	future.then(this, [this, interface, keys = std::move(keys)](QByteArray key) mutable {
		for (auto itr = keys.cbegin(); itr != keys.cend(); ++itr) {
			using KeyIds = QList<QString>;
			KeyIds authenticatableKeys;
			KeyIds authenticatedKeys;

			const auto jid = itr.key();
			const auto trustLevels = itr.value();

			for (auto trustLevelItr = trustLevels.cbegin(); trustLevelItr != trustLevels.cend(); ++trustLevelItr) {
				const auto keyId = trustLevelItr.key();
				const auto trustLevel = trustLevelItr.value();

				if (trustLevel == QXmpp::TrustLevel::Authenticated) {
					authenticatedKeys.append(QString::fromUtf8(keyId.toHex()));
				} else if (trustLevel != QXmpp::TrustLevel::Undecided) {
					authenticatableKeys.append(QString::fromUtf8(keyId.toHex()));
				}
			}

			updateCachedKeys(jid, authenticatableKeys, authenticatedKeys);
		}

		const auto ownJid = AccountManager::instance()->jid();

		OmemoCache::instance()->setAuthenticatedKeys(ownJid, { key.toHex() });

		keys.insert(ownJid, { { key, QXmpp::TrustLevel::Authenticated } });
		Q_EMIT MessageModel::instance()->keysRetrieved(keys);

		interface.reportFinished();
	});

	return interface.future();
}

void OmemoManager::retrieveKeysForRequestedJids(const QList<QString> &jids)
{
	if (std::search(m_lastRequestedKeyOwnerJids.cbegin(), m_lastRequestedKeyOwnerJids.cend(), jids.cbegin(), jids.cend()) != m_lastRequestedKeyOwnerJids.cend()) {
		retrieveKeys(m_lastRequestedKeyOwnerJids);
	}
}

void OmemoManager::retrieveDevicesForRequestedJids(const QString &jid)
{
	if (m_lastRequestedKeyOwnerJids.contains(jid)) {
		retrieveDevices({ jid });
	}
}

void OmemoManager::retrieveDevices(const QList<QString> &jids)
{
	auto future = m_manager->devices(jids);
	future.then(this, [this, jids](QVector<QXmppOmemoDevice> devices) {
		using JidDeviceMap = QMultiHash<QString, Device>;
		JidDeviceMap distrustedDevices;
		JidDeviceMap usableDevices;
		JidDeviceMap authenticatableDevices;
		JidDeviceMap authenticatedDevices;

		for (const auto &device : std::as_const(devices)) {
			const auto jid = device.jid();
			const auto label = device.label();
			const auto keyId = device.keyId();
			const auto trustLevel = device.trustLevel();

			if ((QXmpp::TrustLevel::AutomaticallyDistrusted | QXmpp::TrustLevel::ManuallyDistrusted).testFlag(trustLevel)) {
				distrustedDevices.insert(jid, { label, QString::fromUtf8(keyId.toHex()) });
			} else {
				usableDevices.insert(jid, { label, QString::fromUtf8(keyId.toHex()) });
			}

			if (trustLevel == QXmpp::TrustLevel::Authenticated) {
				authenticatedDevices.insert(jid, { label, QString::fromUtf8(keyId.toHex()) });
			} else if (trustLevel != QXmpp::TrustLevel::Undecided) {
				authenticatableDevices.insert(jid, { label, QString::fromUtf8(keyId.toHex()) });
			}
		}

		for (const auto &jid : jids) {
			updateCachedDevices(jid, distrustedDevices.values(jid), usableDevices.values(jid), authenticatableDevices.values(jid), authenticatedDevices.values(jid));
		}

		const auto ownDevice = m_manager->ownDevice();
		OmemoCache::instance()->setOwnDevice({ ownDevice.label(), QString::fromUtf8(ownDevice.keyId().toHex()) });
	});
}

void OmemoManager::updateCachedKeys(const QString &jid, const QList<QString> &authenticatableKeys, const QList<QString> &authenticatedKeys)
{
	OmemoCache::instance()->setAuthenticatableKeys(jid, authenticatableKeys);
	OmemoCache::instance()->setAuthenticatedKeys(jid, authenticatedKeys);
}

void OmemoManager::updateCachedDevices(const QString &jid, const QList<Device> &distrustedDevices, const QList<Device> &usableDevices, const QList<Device> &authenticatableDevices, const QList<Device> &authenticatedDevices)
{
	OmemoCache::instance()->setDistrustedDevices(jid, distrustedDevices);
	OmemoCache::instance()->setUsableDevices(jid, usableDevices);
	OmemoCache::instance()->setAuthenticatableDevices(jid, authenticatableDevices);
	OmemoCache::instance()->setAuthenticatedDevices(jid, authenticatedDevices);
}
