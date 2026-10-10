// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatInviteeFilterModel.h"

// Kaidan
#include "Account.h"
#include "RosterModel.h"

GroupChatInviteeFilterModel::GroupChatInviteeFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool GroupChatInviteeFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

    const auto jid = sourceModel()->data(index, RosterModel::JidRole).toString();

    return (sourceModel()->data(index, RosterModel::AccountRole).value<Account *>()->settings()->jid() == m_accountJid && !m_groupChatUserJids.contains(jid)
            && !sourceModel()->data(index, RosterModel::IsGroupChatRole).toBool() && jid != m_accountJid)
        && (sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegularExpression())
            || sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegularExpression()));
}

void GroupChatInviteeFilterModel::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        invalidateFilter();
    }
}

void GroupChatInviteeFilterModel::setGroupChatUserJids(const QList<QString> &groupChatUserJids)
{
    if (m_groupChatUserJids != groupChatUserJids) {
        m_groupChatUserJids = groupChatUserJids;
        invalidateFilter();
    }
}

#include "moc_GroupChatInviteeFilterModel.cpp"
