// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUserKeyAuthenticationFilterModel.h"

#include "Algorithms.h"
#include "EncryptionController.h"
#include "FutureUtils.h"
#include "GroupChatUserModel.h"

GroupChatUserKeyAuthenticationFilterModel::GroupChatUserKeyAuthenticationFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    connect(this, &GroupChatUserKeyAuthenticationFilterModel::sourceModelChanged, this, &GroupChatUserKeyAuthenticationFilterModel::setUp);
}

bool GroupChatUserKeyAuthenticationFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    QModelIndex index = model->index(sourceRow, 0, sourceParent);

    const auto jid = model->data(index, GroupChatUserModel::Role::Jid).toString();

    if (!m_jids.contains(jid)) {
        return false;
    }

    return model->data(index, GroupChatUserModel::Role::Name).toString().toLower().contains(filterRegExp()) || jid.toLower().contains(filterRegExp());
}

void GroupChatUserKeyAuthenticationFilterModel::setUp()
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    connect(model, &GroupChatUserModel::userJidsChanged, this, &GroupChatUserKeyAuthenticationFilterModel::updateJids);
    connect(EncryptionController::instance(),
            &EncryptionController::devicesChanged,
            this,
            &GroupChatUserKeyAuthenticationFilterModel::handleDevicesChanged,
            Qt::UniqueConnection);
}

void GroupChatUserKeyAuthenticationFilterModel::handleDevicesChanged(const QString &accountJid, const QList<QString> &jids)
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    const auto userJids = model->userJids();
    const QList<QString> userJidList = {userJids.cbegin(), userJids.cend()};

    if (model->accountJid() == accountJid && containCommonElement(userJidList, jids)) {
        updateJids();
    }
}

void GroupChatUserKeyAuthenticationFilterModel::updateJids()
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    const auto userJids = model->userJids();

    if (userJids.isEmpty()) {
        m_jids.clear();
        invalidateFilter();
        return;
    }

    await(EncryptionController::instance()->devices(model->accountJid(), model->userJids()), this, [this](QList<EncryptionController::Device> &&devices) {
        const auto jids = transformFilter<QList<QString>>(std::as_const(devices), [](const EncryptionController::Device &device) -> std::optional<QString> {
            if (TRUST_LEVEL_AUTHENTICATABLE.testFlag(device.trustLevel)) {
                return device.jid;
            }

            return {};
        });

        if (m_jids != jids) {
            m_jids = jids;
            invalidateFilter();
        }
    });
}
