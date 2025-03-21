// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "HostCompletionModel.h"

// Qt
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>
// Kaidan
#include "Algorithms.h"
#include "Globals.h"
#include "KaidanCoreLog.h"
#include "RosterModel.h"

int HostCompletionModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? m_hosts.size() : 0;
}

QVariant HostCompletionModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column(), index.parent())) {
        switch (role) {
        case Qt::DisplayRole:
        case Qt::ToolTipRole:
        case Qt::WhatsThisRole:
            return m_hosts.at(index.row());
        }
    }

    return {};
}

void HostCompletionModel::clear()
{
    if (rowCount() == 0) {
        return;
    }

    beginResetModel();
    m_hosts.clear();
    endResetModel();
}

void HostCompletionModel::aggregate(const QStringList &jids)
{
    QStringList missing;

    missing.reserve(jids.size());

    for (auto jid : jids) {
        jid = transform(jid);

        if (!m_hosts.contains(jid) && !missing.contains(jid)) {
            missing.append(jid);
        }
    }

    if (missing.isEmpty()) {
        return;
    }

    const auto count = m_hosts.size();

    beginInsertRows({}, count, count + missing.size() - 1);
    m_hosts.append(missing);
    endInsertRows();
}

void HostCompletionModel::aggregateKnownProviders()
{
    QStringList hosts;

    hosts.append(completionProviders());
    hosts.append(rosterProviders());

    aggregate(hosts);
}

RosterModel *HostCompletionModel::rosterModel() const
{
    return m_rosterModel;
}

void HostCompletionModel::setRosterModel(RosterModel *model)
{
    if (m_rosterModel == model) {
        return;
    }

    if (m_rosterModel) {
        disconnect(m_rosterModel, &RosterModel::rowsInserted, this, &HostCompletionModel::aggregateRoster);
        disconnect(m_rosterModel, &RosterModel::layoutChanged, this, &HostCompletionModel::aggregateRoster);
        disconnect(m_rosterModel, &RosterModel::modelReset, this, &HostCompletionModel::aggregateRoster);
    }

    m_rosterModel = model;

    if (m_rosterModel) {
        connect(m_rosterModel, &RosterModel::rowsInserted, this, &HostCompletionModel::aggregateRoster);
        connect(m_rosterModel, &RosterModel::layoutChanged, this, &HostCompletionModel::aggregateRoster);
        connect(m_rosterModel, &RosterModel::modelReset, this, &HostCompletionModel::aggregateRoster);
    }

    Q_EMIT rosterModelChanged(model);
}

QString HostCompletionModel::transform(const QString &entry) const
{
    const int index = entry.indexOf(QStringLiteral("@"));
    return (index == -1 ? entry : entry.mid(index + 1)).toLower();
}

QStringList HostCompletionModel::completionProviders() const
{
    if (!QFile::exists(PROVIDER_COMPLETION_LIST_FILE_PATH)) {
        qCWarning(KAIDAN_CORE_LOG, "Can not open known providers file: %ls, file does not exists", qUtf16Printable(PROVIDER_COMPLETION_LIST_FILE_PATH));
        return {};
    }

    QFile file(PROVIDER_COMPLETION_LIST_FILE_PATH);

    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(KAIDAN_CORE_LOG,
                  "Can not open known providers file: %ls, %ls",
                  qUtf16Printable(PROVIDER_COMPLETION_LIST_FILE_PATH),
                  qUtf16Printable(file.errorString()));
        return {};
    }

    QJsonParseError error;
    const auto document = QJsonDocument::fromJson(file.readAll(), &error);

    if (error.error != QJsonParseError::NoError) {
        qCWarning(KAIDAN_CORE_LOG, "Can not parse known providers JSON file: %ls, %ls", qUtf16Printable(file.fileName()), qUtf16Printable(error.errorString()));
        return {};
    }

    return ::transform<QStringList>(document.array(), [](const QJsonValue &value) {
        return value.toString();
    });
}

QStringList HostCompletionModel::rosterProviders() const
{
    if (m_rosterModel) {
        const auto items = m_rosterModel->items();
        return ::transform<QStringList>(items, [](const RosterItem &item) {
            return item.jid;
        });
    }

    return {};
}

void HostCompletionModel::aggregateRoster()
{
    aggregate(rosterProviders());
}

#include "moc_HostCompletionModel.cpp"
