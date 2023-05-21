// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "DatabaseComponent.h"

#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlQuery>
#include <QSqlRecord>

#include "Database.h"

DatabaseComponent::DatabaseComponent(Database *database, QObject *parent)
	: QObject(parent),
	  m_database(database)
{
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
