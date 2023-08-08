// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SqlUtils.h"
// Qt
#include <QDateTime>
#include <QDebug>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
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
