/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "OmemoManager.h"

#include <QStringBuilder>
#include <QTimer>

#include <QXmppAtmManager.h>
#include <QXmppMamManager.h>
#include <QXmppMessageReceiptManager.h>
#include <QXmppOmemoManager.h>

#include "AccountManager.h"
#include "FutureUtils.h"
#include "MessageModel.h"
#include "OmemoDb.h"
#include "PresenceCache.h"
#include "RosterModel.h"

using namespace std::chrono_literals;

// interval to enable session building for new devices
constexpr auto SESSION_BUILDING_ENABLING_FOR_NEW_DEVICES_TIMER_INTERVAL = 500ms;

OmemoManager::OmemoManager(QXmppClient *client, Database *database, QObject *parent)
	: QObject(parent),
	  m_omemoStorage(new OmemoDb(database, {}, this)),
	  m_manager(client->addNewExtension<QXmppOmemoManager>(m_omemoStorage.get()))
{
	connect(m_manager, &QXmppOmemoManager::trustLevelsChanged, this, [=](const QMultiHash<QString, QByteArray> &modifiedKeys) {
		retrieveKeys(modifiedKeys.keys());
	});

	connect(m_manager, &QXmppOmemoManager::deviceAdded, this, [=](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::deviceChanged, this, [=](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::deviceRemoved, this, [=](const QString &jid, uint32_t) {
		retrieveDevicesForRequestedJids(jid);
	});

	connect(m_manager, &QXmppOmemoManager::devicesRemoved, this, &OmemoManager::retrieveDevicesForRequestedJids);

	connect(m_manager, &QXmppOmemoManager::allDevicesRemoved, this, [=]() {
		for (const auto &jid : std::as_const(m_lastRequestedDeviceJids)) {
			emitDeviceSignals(jid, {}, {}, {});
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

	auto future = m_manager->setSecurityPolicy(QXmpp::TrustSecurityPolicy::Toakafa);
	await(future, this, [=]() mutable {
		auto future = m_manager->changeDeviceLabel(APPLICATION_DISPLAY_NAME % QStringLiteral(" - ") % QSysInfo::prettyProductName());
		await(future, this, [=](bool) mutable {
			auto future = m_manager->load();
			await(future, this, [=](bool isLoaded) mutable {
				m_isLoaded = isLoaded;
				interface.reportFinished();
			});
		});
	});

	return interface.future();
}

QFuture<void> OmemoManager::setUp()
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	if (m_isLoaded) {
		enableSessionBuildingForNewDevices();
	} else {
		auto future = m_manager->setUp();
		await(future, this, [=](bool isSetUp) mutable {
			if (!isSetUp) {
				emit Kaidan::instance()->passiveNotificationRequested(tr("Secure conversations are not possible because OMEMO could not be set up"));
				interface.reportFinished();
				return;
			} else {
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
				await(future, this, [=]() mutable {
					interface.reportFinished();
				});
			}
		});
	}

	return interface.future();
}

QFuture<void> OmemoManager::retrieveKeys(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->keys(jids, ~ QXmpp::TrustLevels { QXmpp::TrustLevel::Undecided });
	await(future, this, [=](QHash<QString, QHash<QByteArray, QXmpp::TrustLevel>> &&keys) mutable {
		auto future = retrieveOwnKey(std::move(keys));
		await(future, this, [=]() mutable {
			interface.reportFinished();
		});
	});

	return interface.future();
}

QFuture<void> OmemoManager::requestDeviceLists(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->requestDeviceLists(jids);
	await(future, this, [=]() mutable {
		interface.reportFinished();
	});

	return interface.future();
}

QFuture<void> OmemoManager::subscribeToDeviceLists(const QList<QString> &jids)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->subscribeToDeviceLists(jids);
	await(future, this, [=]() mutable {
		interface.reportFinished();
	});

	return interface.future();
}

QFuture<void> OmemoManager::unsubscribeFromDeviceLists()
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	auto future = m_manager->unsubscribeFromDeviceLists();
	await(future, this, [=]() mutable {
		interface.reportFinished();
	});

	return interface.future();
}

void OmemoManager::resetOwnDevice()
{
	m_manager->resetOwnDevice();
}

void OmemoManager::enableSessionBuildingForNewDevices()
{
	m_manager->setNewDeviceAutoSessionBuildingEnabled(true);
}

QFuture<void> OmemoManager::initializeChat(const QString &accountJid, const QString &chatJid)
{
	QFutureInterface<void> interface(QFutureInterfaceBase::Started);

	const QList<QString> jids = { accountJid, chatJid };
	m_lastRequestedDeviceJids = jids;

	auto initializeSessionsKeysAndDevices = [this, interface, jids]() mutable {
		auto future = m_manager->buildMissingSessions(jids);
		await(future, this, [=]() mutable {
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
		}, this, [=](std::tuple<bool, int> result) mutable {
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
	await(future, this, [interface, keys = std::move(keys)](QByteArray key) mutable {
		keys.insert(AccountManager::instance()->jid(), { { key, QXmpp::TrustLevel::Authenticated } });
		emit MessageModel::instance()->keysRetrieved(keys);
		interface.reportFinished();
	});

	return interface.future();
}

void OmemoManager::retrieveDevicesForRequestedJids(const QString &jid)
{
	if (m_lastRequestedDeviceJids.contains(jid)) {
		retrieveDevices({ jid });
	}
}

void OmemoManager::retrieveDevices(const QList<QString> &jids)
{
	auto future = m_manager->devices(jids);
	await(future, this, [=](QVector<QXmppOmemoDevice> devices) {
		using JidLabelMap = QMultiHash<QString, QString>;
		JidLabelMap distrustedDevices;
		JidLabelMap usableDevices;
		JidLabelMap authenticatableDevices;

		for (const auto &device : std::as_const(devices)) {
			const auto jid = device.jid();
			const auto trustLevel = device.trustLevel();
			const auto label = device.label();

			if ((QXmpp::TrustLevel::AutomaticallyDistrusted | QXmpp::TrustLevel::ManuallyDistrusted).testFlag(trustLevel)) {
				distrustedDevices.insert(jid, label);
			} else {
				usableDevices.insert(jid, label);
			}

			if (!((QXmpp::TrustLevel::Undecided | QXmpp::TrustLevel::Authenticated).testFlag(trustLevel))) {
				authenticatableDevices.insert(jid, label);
			}
		}

		for (const auto &jid : jids) {
			emitDeviceSignals(jid, distrustedDevices.values(jid), usableDevices.values(jid), authenticatableDevices.values(jid));
		}
	});
}

void OmemoManager::emitDeviceSignals(const QString &jid, const QList<QString> &distrustedDevices, const QList<QString> &usableDevices, const QList<QString> &authenticatableDevices)
{
	emit MessageModel::instance()->distrustedOmemoDevicesRetrieved(jid, distrustedDevices);
	emit MessageModel::instance()->usableOmemoDevicesRetrieved(jid, usableDevices);
	emit MessageModel::instance()->authenticatableOmemoDevicesRetrieved(jid, authenticatableDevices);
}