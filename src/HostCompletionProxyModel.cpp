// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "HostCompletionProxyModel.h"

HostCompletionProxyModel::HostCompletionProxyModel(QObject *parent)
    : QSortFilterProxyModel{parent}
{
    setSortCaseSensitivity(Qt::CaseInsensitive);

    connect(this, &HostCompletionProxyModel::userInputChanged, this, &HostCompletionProxyModel::invalidateFilter);
}

QVariant HostCompletionProxyModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        if (role == filterRole()) {
            const auto sourceIndex = mapToSource(index);
            return QStringLiteral("%1@%2").arg(prefix(), sourceIndex.data(filterRole()).toString());
        }
    }

    return QSortFilterProxyModel::data(index, role);
}

const QString &HostCompletionProxyModel::userInput() const
{
    return m_userInput;
}

void HostCompletionProxyModel::setUserInput(const QString &newUserInput)
{
    if (m_userInput != newUserInput) {
        m_userInput = newUserInput;
        Q_EMIT userInputChanged(newUserInput);
    }
}

bool HostCompletionProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    if (!m_userInput.contains(QStringLiteral("@"))) {
        return false;
    }

    const auto sourceIndex = sourceModel()->index(sourceRow, 0, sourceParent);
    const auto value = sourceIndex.data(filterRole()).toString();
    const auto currentDomain = HostCompletionProxyModel::domain();
    return value.startsWith(currentDomain, filterCaseSensitivity()) && value.size() != currentDomain.size();
}

QString HostCompletionProxyModel::prefix() const
{
    const QString input = userInput();
    const int index = input.indexOf(QStringLiteral("@"));
    return index == -1 ? QString() : input.left(index);
}

QString HostCompletionProxyModel::domain() const
{
    const QString input = userInput();
    const int index = input.indexOf(QStringLiteral("@"));
    return index == -1 ? QString() : input.mid(index + 1);
}

#include "moc_HostCompletionProxyModel.cpp"
