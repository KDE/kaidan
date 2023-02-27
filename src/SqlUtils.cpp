/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
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

#include "SqlUtils.h"
// Qt
#include <QDateTime>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlQuery>
#include <QSqlRecord>

namespace SqlUtils {

void prepareQuery(QSqlQuery &query, const QString &sql)
{
	if (!query.prepare(sql)) {
		qDebug() << "Failed to prepare query:" << sql;
		qFatal("QSqlError: %s", qPrintable(query.lastError().text()));
	}
}

void bindValues(QSqlQuery &query, const std::vector<QVariant> &values)
{
	for (const auto &val : values) {
		query.addBindValue(val);
	}
}

void bindValues(QSqlQuery &query, const std::vector<QueryBindValue> &values)
{
	for (const auto &bindValue : values) {
		query.bindValue(bindValue.key.toString(), bindValue.value);
	}
}

void execQuery(QSqlQuery &query)
{
	if (!query.exec()) {
		qDebug() << "Failed to execute query:" << query.executedQuery();
		qFatal("QSqlError: %s", qPrintable(query.lastError().text()));
	}
}

void execQuery(QSqlQuery &query, const QString &sql)
{
	prepareQuery(query, sql);
	execQuery(query);
}

void execQuery(QSqlQuery &query,
               const QString &sql,
               const QVector<QVariant> &bindValues)
{
	prepareQuery(query, sql);

	for (const auto &val : bindValues)
		query.addBindValue(val);

	execQuery(query);
}

QSqlField createSqlField(const QString &key, const QVariant &val)
{
	QSqlField field(key, val.type());
	field.setValue(val);
	return field;
}

QString simpleWhereStatement(const QSqlDriver *driver,
                             const QString &key,
                             const QVariant &val)
{
	QSqlRecord rec;
	rec.append(createSqlField(key, val));

	return " " + driver->sqlStatement(
	        QSqlDriver::WhereStatement,
	        QString(),
	        rec,
	        false
	);
}

QString simpleWhereStatement(const QSqlDriver *driver, const QMap<QString, QVariant> &keyValuePairs)
{
	QSqlRecord rec;

	const auto keys = keyValuePairs.keys();
	for (const QString &key : keys)
		rec.append(createSqlField(key, keyValuePairs.value(key)));

	return " " + driver->sqlStatement(
		QSqlDriver::WhereStatement,
		{},
		rec,
		false
	);
}

QVariant serialize(const std::optional<QDateTime> &dateTime)
{
	if (dateTime) {
		return dateTime->toMSecsSinceEpoch();
	}
	return QVariant(QVariant::LongLong);
}

QVariant serialize(const QDateTime &dateTime)
{
	if (dateTime.isValid()) {
		return dateTime.toMSecsSinceEpoch();
	}
	return QVariant(0LL);
}

std::optional<QDateTime> parseOptDateTime(QSqlQuery &query, int index)
{
	if (auto value = query.value(index); !value.isNull()) {
		Q_ASSERT(value.type() == QVariant::LongLong);
		if (auto integer = value.toLongLong(); integer != 0) {
			return QDateTime::fromMSecsSinceEpoch(integer);
		}
	}
	return {};
}

QDateTime parseDateTime(QSqlQuery &query, int index)
{
	return parseOptDateTime(query, index).value_or(QDateTime());
}
}
