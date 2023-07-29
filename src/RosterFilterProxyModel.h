// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QSortFilterProxyModel>

class RosterFilterProxyModel : public QSortFilterProxyModel
{
	Q_OBJECT

	Q_PROPERTY(QVector<QString> selectedAccountJids READ selectedAccountJids WRITE setSelectedAccountJids NOTIFY selectedAccountJidsChanged)
	Q_PROPERTY(QVector<QString> selectedGroups READ selectedGroups WRITE setSelectedGroups NOTIFY selectedGroupsChanged)

public:
	RosterFilterProxyModel(QObject *parent = nullptr);

	void setSelectedAccountJids(const QVector<QString> &selectedAccountJids);
	QVector<QString> selectedAccountJids() const;
	Q_SIGNAL void selectedAccountJidsChanged();

	void setSelectedGroups(const QVector<QString> &selectedGroups);
	QVector<QString> selectedGroups() const;
	Q_SIGNAL void selectedGroupsChanged();

	bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
	QVector<QString> m_selectedAccountJids;
	QVector<QString> m_selectedGroups;
};
