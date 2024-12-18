// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUserFilterModel.h"

#include "GroupChatUserModel.h"

GroupChatUserFilterModel::GroupChatUserFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool GroupChatUserFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = static_cast<GroupChatUserModel *>(sourceModel());
    QModelIndex index = model->index(sourceRow, 0, sourceParent);

    return model->data(index, GroupChatUserModel::Role::Name).toString().toLower().contains(filterRegularExpression())
        || model->data(index, GroupChatUserModel::Role::Jid).toString().toLower().contains(filterRegularExpression());
}

#include "moc_GroupChatUserFilterModel.cpp"
