/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include <QStringView>
#include <QVariant>
#include <optional>
#include <vector>

class QSqlDriver;
class QSqlField;
class QSqlQuery;

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

}  // SqlUtils
