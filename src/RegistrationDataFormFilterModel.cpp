// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RegistrationDataFormFilterModel.h"
#include "RegistrationDataFormModel.h"

RegistrationDataFormFilterModel::RegistrationDataFormFilterModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
}

bool RegistrationDataFormFilterModel::filterAcceptsRow(int sourceRow, const QModelIndex &) const
{
    return !m_filteredRows.contains(sourceRow);
}

void RegistrationDataFormFilterModel::setSourceModel(QAbstractItemModel *sourceModel)
{
    m_filteredRows.clear();

    auto *dataFormModel = static_cast<RegistrationDataFormModel *>(sourceModel);
    if (dataFormModel) {
        m_filteredRows = dataFormModel->indiciesToFilter();
    }

    QSortFilterProxyModel::setSourceModel(sourceModel);
    Q_EMIT isEmptyChanged();
}

bool RegistrationDataFormFilterModel::isEmpty()
{
    if (sourceModel()) {
        return m_filteredRows.size() == static_cast<RegistrationDataFormModel *>(sourceModel())->rowCount();
    }

    return true;
}

#include "moc_RegistrationDataFormFilterModel.cpp"
