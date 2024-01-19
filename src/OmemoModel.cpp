// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "OmemoModel.h"

#include "Kaidan.h"
#include "OmemoCache.h"

OmemoModel::OmemoModel(QObject *parent)
	: QAbstractListModel(parent)
{
}

OmemoModel::~OmemoModel()
{
}

int OmemoModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid()) {
		return 0;
	}

	return m_ownDevice ? m_devices.size() + 1 : m_devices.size();
}

QHash<int, QByteArray> OmemoModel::roleNames() const
{
	static const QHash<int, QByteArray> roles = {
		{ static_cast<int>(Role::Label), QByteArrayLiteral("label") },
		{ static_cast<int>(Role::KeyId), QByteArrayLiteral("keyId") },
	};

	return roles;
}

QVariant OmemoModel::data(const QModelIndex &index, int role) const
{
	const auto row = index.row();

	if (!hasIndex(row, index.column(), index.parent())) {
		return {};
	}

	const OmemoManager::Device *device = nullptr;

	if (m_ownDevice) {
		if (row == 0) {
			device = &(*m_ownDevice);
		} else {
			device = &m_devices.at(row - 1);
		}
	} else {
		device = &m_devices.at(row);
	}

	switch (static_cast<Role>(role)) {
	case Role::Label:
		return device->label;
	case Role::KeyId:
		return device->keyId;
	}

	return {};
}

void OmemoModel::setJid(const QString &jid)
{
	if (m_jid != jid) {
		m_jid = jid;
		setUp();
	}
}

void OmemoModel::setOwnAuthenticatedKeysProcessed(bool ownAuthenticatedKeysProcessed)
{
	if (m_ownAuthenticatedKeysProcessed != ownAuthenticatedKeysProcessed) {
		m_ownAuthenticatedKeysProcessed = ownAuthenticatedKeysProcessed;

		if (!m_jid.isEmpty()) {
			setUp();
		}
	}

}

bool OmemoModel::contains(const QString &keyId)
{
	return std::any_of(m_devices.cbegin(), m_devices.cend(), [keyId](const OmemoManager::Device &device) {
		return device.keyId == keyId;
	});
}

void OmemoModel::setUp()
{
	disconnect(Kaidan::instance()->client()->caches()->omemoCache, nullptr, this, nullptr);

	if (m_ownAuthenticatedKeysProcessed) {
		connect(Kaidan::instance()->client()->caches()->omemoCache, &OmemoCache::ownDeviceUpdated, this, [this](const OmemoManager::Device &ownDevice) {
			setOwnDevice(ownDevice);
		});

		setOwnDevice(OmemoCache::instance()->ownDevice());

		connect(Kaidan::instance()->client()->caches()->omemoCache, &OmemoCache::authenticatedDevicesUpdated, this, [this](const QString &jid, const QList<OmemoManager::Device> &authenticatedDevices) {
			if (jid == m_jid) {
				setDevices(authenticatedDevices);
			}
		});

		setDevices(OmemoCache::instance()->authenticatedDevices(m_jid));
	} else {
		connect(Kaidan::instance()->client()->caches()->omemoCache, &OmemoCache::authenticatableDevicesUpdated, this, [this](const QString &jid, const QList<OmemoManager::Device> &authenticatableDevices) {
			if (jid == m_jid) {
				setDevices(authenticatableDevices);
			}
		});

		setDevices(OmemoCache::instance()->authenticatableDevices(m_jid));
	}
}

void OmemoModel::setOwnDevice(OmemoManager::Device ownDevice)
{
	ownDevice.label += " · " + tr("This device");
	beginInsertRows(QModelIndex(), 0, 0);
	m_ownDevice = ownDevice;
	endInsertRows();
}

void OmemoModel::setDevices(const QList<OmemoManager::Device> &devices)
{
	beginResetModel();
	m_devices = devices;
	endResetModel();
}
