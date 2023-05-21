// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class PublicGroupChatProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(QString languageFilter READ languageFilter WRITE setLanguageFilter NOTIFY languageFilterChanged)
	Q_PROPERTY(int count READ count NOTIFY countChanged)

public:
	explicit PublicGroupChatProxyModel(QObject *parent = nullptr);

	void setSourceModel(QAbstractItemModel *sourceModel) override;
	Q_INVOKABLE void sort(int column, Qt::SortOrder order = Qt::AscendingOrder) override;

	const QString &languageFilter() const;
	void setLanguageFilter(const QString &language);
	Q_SIGNAL void languageFilterChanged(const QString &language);

	int count() const;
	Q_SIGNAL void countChanged();

protected:
	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	QString m_languageFilter;
};
