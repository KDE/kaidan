// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QStringView>
#include <QSqlQuery>
#include <QVariant>

#include <optional>
#include <vector>

class QSqlDriver;
class QSqlField;

namespace SqlUtils {

/// Key-value pair to be bound to a SqlQuery
struct QueryBindValue
{
	QStringView key;
	QVariant value;
};

/**
 * Prepares an SQL query for executing it by @c execQuery and handles possible
 * errors.
 *
 * @param query SQL query
 * @param sql SQL statement
 */
void prepareQuery(QSqlQuery &query, const QString &sql);

void bindValues(QSqlQuery &query, const std::vector<QVariant> &values);
void bindValues(QSqlQuery &query, const std::vector<QueryBindValue> &values);

/**
 * Executes an SQL query and handles possible errors.
 *
 * @param query SQL query
 */
void execQuery(QSqlQuery &query);

/**
 * Prepares an SQL query, executes it and handles possible errors.
 *
 * @param query SQL query
 * @param sql SQL statement
 */
void execQuery(QSqlQuery &query, const QString &sql);

/**
 * Prepares an SQL query, sequentially binds values, executes the query and
 * handles possible errors.
 *
 * @param query SQL query
 * @param sql SQL statement
 * @param bindValues values to be bound sequentially
 */
inline void execQuery(QSqlQuery &query, const QString &sql, const std::vector<QVariant> &values)
{
	prepareQuery(query, sql);
	bindValues(query, values);
	execQuery(query);
}

/**
 * Prepares an SQL query, binds values by names, executes the query and handles
 * possible errors.
 *
 * @param query SQL query
 * @param sql SQL statement
 * @param bindValues values to be bound as key-value pairs
 */
inline void execQuery(QSqlQuery &query, const QString &sql, const std::vector<QueryBindValue> &values)
{
	prepareQuery(query, sql);
	bindValues(query, values);
	execQuery(query);
}

/**
 * Creates an SQL field that may be used for an SQL statement.
 *
 * @param key name of the SQL field
 * @param val value of the SQL field
 * @return the SQL field.
 */
QSqlField createSqlField(const QString &key, const QVariant &val);

/**
 * Creates a where clause with one parameter.
 *
 * @param driver SQL database driver
 * @param key name of the where condition
 * @param val value of the where condition
 * @return the where clause with a space, so it can be directly appended to
 *         another statement.
 */
QString simpleWhereStatement(const QSqlDriver *driver, const QString &key, const QVariant &val);

/**
 * Creates a where clause with multiple key value pairs.
 *
 * @param driver SQL database driver
 * @param keyValuePairs key-value pairs of the where clause
 * @return a where clause starting with a space, so it can be directly
 *         appended to another statement
 */
QString simpleWhereStatement(const QSqlDriver *driver, const QMap<QString, QVariant> &keyValuePairs);

/// Serialize optional<QDateTime> to nullable INTEGER
QVariant serialize(const std::optional<QDateTime> &dateTime);
/// Serialize QDateTime to 'INTEGER NOT NULL'
QVariant serialize(const QDateTime &dateTime);

/// Parse optional<QDateTime> from nullable INTEGER
std::optional<QDateTime> parseOptDateTime(QSqlQuery &query, int index);
/// Parse QDateTime from 'INTEGER NOT NULL'
QDateTime parseDateTime(QSqlQuery &query, int index);

/// Try to reserve space for a query in a container.
template<typename Container>
void reserve(Container &container, const QSqlQuery &query)
{
	if (!query.isActive()) {
		return;
	}

	if (query.isSelect()) {
		if (query.size() != -1) {
			container.reserve(query.size());
		}
	}
}

}  // SqlUtils
