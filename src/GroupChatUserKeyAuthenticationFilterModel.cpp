// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUserKeyAuthenticationFilterModel.h"

// Kaidan
#include "AccountController.h"
#include "Algorithms.h"
#include "EncryptionController.h"
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

    return model->data(index, GroupChatUserModel::Role::Name).toString().toLower().contains(filterRegularExpression())
        || jid.toLower().contains(filterRegularExpression());
}

void GroupChatUserKeyAuthenticationFilterModel::setUp()
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());

    connect(model, &GroupChatUserModel::accountJidChanged, this, [this, model]() {
        const auto accountJid = model->accountJid();

        if (accountJid.isEmpty()) {
            return;
        }

        m_encryptionController = AccountController::instance()->account(accountJid)->encryptionController();

        connect(model, &GroupChatUserModel::userJidsChanged, this, &GroupChatUserKeyAuthenticationFilterModel::updateJids);
        connect(m_encryptionController,
                &EncryptionController::devicesChanged,
                this,
                &GroupChatUserKeyAuthenticationFilterModel::handleDevicesChanged,
                Qt::UniqueConnection);
    });
}

void GroupChatUserKeyAuthenticationFilterModel::handleDevicesChanged(const QList<QString> &jids)
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    const auto userJids = model->userJids();

    if (containCommonElement(userJids, jids)) {
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

    m_encryptionController->devices(model->userJids()).then([this](QList<EncryptionController::Device> &&devices) {
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

#include "moc_GroupChatUserKeyAuthenticationFilterModel.cpp"
