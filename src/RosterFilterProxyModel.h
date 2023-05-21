// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class RosterFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	RosterFilterProxyModel(QObject *parent = nullptr);

	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
