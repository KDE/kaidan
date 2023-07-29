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

void RosterFilterProxyModel::setSelectedAccountJids(const QVector<QString> &selectedAccountJids)
{
	m_selectedAccountJids = selectedAccountJids;
	Q_EMIT selectedAccountJidsChanged();
	invalidate();
}

QVector<QString> RosterFilterProxyModel::selectedAccountJids() const
{
	return m_selectedAccountJids;
}

void RosterFilterProxyModel::setSelectedGroups(const QVector<QString> &selectedGroups)
{
	m_selectedGroups = selectedGroups;
	Q_EMIT selectedGroupsChanged();
	invalidate();
}

QVector<QString> RosterFilterProxyModel::selectedGroups() const
{
	return m_selectedGroups;
}

bool RosterFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	QModelIndex index = sourceModel()->index(sourceRow, 0, sourceParent);

	const auto accountJid = sourceModel()->data(index, RosterModel::AccountJidRole).value<QString>();
	bool accountJidSelected = m_selectedAccountJids.isEmpty() || m_selectedAccountJids.contains(accountJid);

	const auto groups = sourceModel()->data(index, RosterModel::GroupsRole).value<QVector<QString>>();
	bool groupSelected = m_selectedGroups.isEmpty() || std::any_of(groups.cbegin(), groups.cend(), [&](const QString &group) {
		return m_selectedGroups.contains(group);
	});

	return (sourceModel()->data(index, RosterModel::NameRole).toString().toLower().contains(filterRegExp()) ||
			sourceModel()->data(index, RosterModel::JidRole).toString().toLower().contains(filterRegExp())) &&
			accountJidSelected && groupSelected;
}
