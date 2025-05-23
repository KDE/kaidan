// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QSortFilterProxyModel>

/**
 * This class is used to filter the data of the registration form.
 */
class RegistrationDataFormFilterModel : public QSortFilterProxyModel
{
    Q_OBJECT
    Q_PROPERTY(bool isEmpty READ isEmpty NOTIFY isEmptyChanged)

public:
    explicit RegistrationDataFormFilterModel(QObject *parent = nullptr);

    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    void setSourceModel(QAbstractItemModel *sourceModel) override;

    bool isEmpty();
    Q_SIGNAL void isEmptyChanged();

private:
    QList<int> m_filteredRows;
};
