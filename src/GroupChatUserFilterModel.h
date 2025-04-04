// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>

class GroupChatUserFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    explicit GroupChatUserFilterModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
