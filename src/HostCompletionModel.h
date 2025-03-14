// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>

class RosterModel;

class HostCompletionModel : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(RosterModel *rosterModel READ rosterModel WRITE setRosterModel NOTIFY rosterModelChanged)

public:
    using QAbstractListModel::QAbstractListModel;

    int rowCount(const QModelIndex &parent = {}) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;

    Q_SLOT void clear();
    Q_SLOT void aggregate(const QStringList &jids);
    Q_SLOT void aggregateKnownProviders();

    RosterModel *rosterModel() const;
    void setRosterModel(RosterModel *model);
    Q_SIGNAL void rosterModelChanged(RosterModel *model);

private:
    QString transform(const QString &entry) const;
    QStringList completionProviders() const;
    QStringList rosterProviders() const;
    void aggregateRoster();

private:
    QStringList m_hosts;
    RosterModel *m_rosterModel = nullptr;
};
