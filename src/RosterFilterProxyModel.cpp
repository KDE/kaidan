// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterFilterProxyModel.h"
#include "RosterModel.h"

RosterFilterProxyModel::RosterFilterProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
}

bool RosterFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	return sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegExp()) ||
		sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegExp());
}
