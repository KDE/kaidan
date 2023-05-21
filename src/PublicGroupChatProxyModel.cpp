// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PublicGroupChatProxyModel.h"
#include "PublicGroupChatModel.h"

PublicGroupChatProxyModel::PublicGroupChatProxyModel(QObject *parent)
	: QSortFilterProxyModel(parent)
{
	sort(0, Qt::AscendingOrder);

	connect(this, &PublicGroupChatProxyModel::languageFilterChanged, this, &PublicGroupChatProxyModel::invalidateFilter);
	connect(this, &PublicGroupChatProxyModel::rowsInserted, this, &PublicGroupChatProxyModel::countChanged);
	connect(this, &PublicGroupChatProxyModel::rowsRemoved, this, &PublicGroupChatProxyModel::countChanged);
	connect(this, &PublicGroupChatProxyModel::layoutChanged, this, &PublicGroupChatProxyModel::countChanged);
	connect(this, &PublicGroupChatProxyModel::modelReset, this, &PublicGroupChatProxyModel::countChanged);
}

void PublicGroupChatProxyModel::setSourceModel(QAbstractItemModel *sourceModel)
{
	Q_ASSERT(!sourceModel || qobject_cast<PublicGroupChatModel *>(sourceModel));
	QSortFilterProxyModel::setSourceModel(sourceModel);
}

void PublicGroupChatProxyModel::sort(int column, Qt::SortOrder order)
{
	QSortFilterProxyModel::sort(column, order);
}

const QString &PublicGroupChatProxyModel::languageFilter() const
{
	return m_languageFilter;
}

void PublicGroupChatProxyModel::setLanguageFilter(const QString &language)
{
	if (m_languageFilter != language) {
		m_languageFilter = language;
		Q_EMIT languageFilterChanged(m_languageFilter);
	}
}

int PublicGroupChatProxyModel::count() const
{
	return rowCount();
}

bool PublicGroupChatProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
	const QModelIndex sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
	const PublicGroupChat groupChat =
		sourceIndex.data(static_cast<int>(PublicGroupChatModel::CustomRole::GroupChat)).value<PublicGroupChat>();

	if (!(m_languageFilter.isEmpty() || groupChat.languages().contains(m_languageFilter))) {
		return false;
	}

	if (filterRole() == static_cast<int>(PublicGroupChatModel::CustomRole::GlobalSearch)) {
		return groupChat.name().contains(filterRegExp()) ||
		       groupChat.description().contains(filterRegExp()) ||
		       groupChat.address().contains(filterRegExp());
	}

	return QSortFilterProxyModel::filterAcceptsRow(sourceRow, sourceParent);
}
