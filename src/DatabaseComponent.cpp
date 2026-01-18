// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DatabaseComponent.h"

// Qt
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>
// Kaidan
#include "Database.h"

DatabaseComponent::DatabaseComponent(QObject *parent)
    : QObject(parent)
    , m_database(Database::instance())
{
}

void DatabaseComponent::insert(const QString &tableName, const SqlUtils::QueryBindValues &values)
{
    auto query = createQuery();

    QSqlRecord record;
    SqlUtils::addFieldsToRecord(record, values);

    SqlUtils::execQuery(query, sqlDriver().sqlStatement(QSqlDriver::InsertStatement, tableName, record, false));
}

void DatabaseComponent::insertBinary(const QString &tableName, const SqlUtils::QueryBindValues &values)
{
    auto query = createQuery();
    QSqlRecord record;

    // Since "sqlDriver().sqlStatement()" returns a QString, binary data must be bound to it later
    // after preparing the query with placeholders.
    SqlUtils::addPreparedFieldsToRecord(record, values.keys());
    SqlUtils::execQueryWithOrderedValues(query, sqlDriver().sqlStatement(QSqlDriver::InsertStatement, tableName, record, true), values.values());
}

QSqlQuery DatabaseComponent::createQuery()
{
    return m_database->createQuery();
}

QSqlDriver &DatabaseComponent::sqlDriver()
{
    return *m_database->currentDatabase().driver();
}

QSqlRecord DatabaseComponent::sqlRecord(const QString &tableName)
{
    return m_database->currentDatabase().record(tableName);
}

void DatabaseComponent::transaction()
{
    m_database->transaction();
}

void DatabaseComponent::commit()
{
    m_database->commit();
}

QObject *DatabaseComponent::dbWorker() const
{
    return m_database->dbWorker();
}

#include "moc_DatabaseComponent.cpp"
