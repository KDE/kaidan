// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

/**
 * This class is used to filter the data of the registration form.
 */
class RegistrationDataFormFilterModel : public QSortFilterProxyModel
{
	Q_OBJECT

public:
	RegistrationDataFormFilterModel(QObject *parent = nullptr);

	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

	void setSourceModel(QAbstractItemModel *sourceModel) override;

private:
	QVector<int> m_filteredRows;
};
