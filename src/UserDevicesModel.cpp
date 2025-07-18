// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "UserDevicesModel.h"

// QXmpp
#include <QXmppUtils.h>
#include <QXmppVersionIq.h>
// Kaidan
#include "VersionController.h"

UserDevicesModel::UserDevicesModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

QHash<int, QByteArray> UserDevicesModel::roleNames() const
{
    return {{Resource, QByteArrayLiteral("resource")},
            {Version, QByteArrayLiteral("version")},
            {Name, QByteArrayLiteral("name")},
            {OS, QByteArrayLiteral("os")}};
}

QVariant UserDevicesModel::data(const QModelIndex &index, int role) const
{
    Q_ASSERT(checkIndex(index, QAbstractItemModel::CheckIndexOption::IndexIsValid | QAbstractItemModel::CheckIndexOption::ParentIsInvalid));

    switch (role) {
    case Resource:
        return m_devices.at(index.row()).resource;
    case Version:
        return m_devices.at(index.row()).version;
    case Name:
        return m_devices.at(index.row()).name;
    case OS:
        return m_devices.at(index.row()).os;
    }

    Q_UNREACHABLE();
}

int UserDevicesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_devices.size();
}

void UserDevicesModel::setVersionController(VersionController *versionController)
{
    if (m_versionController != versionController) {
        if (m_versionController) {
            m_versionController->disconnect(this);
        }

        m_versionController = versionController;

        connect(m_versionController, &VersionController::clientVersionReceived, this, &UserDevicesModel::handleClientVersionReceived);
    }
}

void UserDevicesModel::setPresenceCache(PresenceCache *presenceCache)
{
    if (m_presenceCache != presenceCache) {
        if (m_presenceCache) {
            m_presenceCache->disconnect(this);
        }

        m_presenceCache = presenceCache;

        connect(m_presenceCache, &PresenceCache::presenceChanged, this, &UserDevicesModel::handlePresenceChanged);
        connect(m_presenceCache, &PresenceCache::presencesCleared, this, &UserDevicesModel::handlePresencesCleared);
    }
}

QString UserDevicesModel::jid() const
{
    return m_jid;
}

void UserDevicesModel::setJid(const QString &jid)
{
    m_jid = jid;

    // Clear data when jid of the model changes
    beginResetModel();
    m_devices.clear();

    // Add DeviceInfo objects for each resource
    const auto resources = m_presenceCache->resources(jid);
    m_devices.reserve(resources.size());

    std::transform(resources.cbegin(), resources.cend(), std::back_inserter(m_devices), [](const QString &resource) {
        return DeviceInfo(resource);
    });
    endResetModel();

    // request version data for all available resources
    m_versionController->fetchVersions(m_jid, {});

    Q_EMIT jidChanged();
}

void UserDevicesModel::handleClientVersionReceived(const QXmppVersionIq &versionIq)
{
    // Only collect version IQs that belong to our jid and that we do not already have
    if (QXmppUtils::jidToBareJid(versionIq.from()) != m_jid)
        return;

    // find the VersionInfo and update values
    const auto resource = QXmppUtils::jidToResource(versionIq.from());
    auto info = std::find_if(m_devices.begin(), m_devices.end(), [&](const auto &device) {
        return device.resource == resource;
    });
    if (info != m_devices.end()) {
        info->name = versionIq.name();
        info->version = versionIq.version();
        info->os = versionIq.os();

        const int i = std::distance(m_devices.begin(), info);
        Q_EMIT dataChanged(index(i), index(i), {Name, Version, OS});
    }
}

void UserDevicesModel::handlePresenceChanged(PresenceCache::ChangeType type, const QString &jid, const QString &resource)
{
    if (jid != m_jid)
        return;

    switch (type) {
    case PresenceCache::Connected:
        beginInsertRows({}, m_devices.size(), m_devices.size());
        m_devices.append(DeviceInfo(resource));
        endInsertRows();

        m_versionController->fetchVersions(m_jid, resource);
        break;
    case PresenceCache::Disconnected:
        for (int i = 0; i < m_devices.size(); i++) {
            if (m_devices.at(i).resource == resource) {
                beginRemoveRows({}, i, i);
                m_devices.removeAt(i);
                endRemoveRows();
                break;
            }
        }
        break;
    case PresenceCache::Updated:
        // do nothing: presence updates don't imply version change
        break;
    }
}

void UserDevicesModel::handlePresencesCleared()
{
    beginResetModel();
    m_devices.clear();
    endResetModel();
}

UserDevicesModel::DeviceInfo::DeviceInfo(const QString &resource)
    : resource(resource)
{
}

UserDevicesModel::DeviceInfo::DeviceInfo(const QXmppVersionIq &iq)
    : resource(QXmppUtils::jidToResource(iq.from()))
    , name(iq.name())
    , version(iq.version())
    , os(iq.os())
{
}

#include "moc_UserDevicesModel.cpp"
