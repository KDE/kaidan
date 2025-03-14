// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "SqlUtils.h"

// Qt
#include <QDateTime>
#include <QSqlDriver>
#include <QSqlError>
#include <QSqlField>
#include <QSqlRecord>
// Kaidan
#include "kaidan_core_debug.h"

namespace SqlUtils
{

void prepareQuery(QSqlQuery &query, const QString &sql)
{
    if (!query.prepare(sql)) {
        qCDebug(KAIDAN_CORE_LOG) << "Failed to prepare query:" << sql;
        qFatal("QSqlError: %s", qPrintable(query.lastError().text()));
    }
}

void bindValues(QSqlQuery &query, const QueryBindValues &values)
{
    for (auto itr = values.cbegin(); itr != values.cend(); ++itr) {
        query.bindValue(itr.key().toString(), *itr);
    }
}

void bindOrderedValues(QSqlQuery &query, const QList<QVariant> &values)
{
    for (const auto &value : values) {
        query.addBindValue(value);
    }
}

void execQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        qCDebug(KAIDAN_CORE_LOG) << "Failed to execute query:" << query.executedQuery();
        qFatal("QSqlError: %s", qPrintable(query.lastError().text()));
    }
}

void execQuery(QSqlQuery &query, const QString &sql)
{
    prepareQuery(query, sql);
    execQuery(query);
}

void addFieldsToRecord(QSqlRecord &record, const QueryBindValues &values)
{
    for (auto itr = values.cbegin(); itr != values.cend(); ++itr) {
        record.append(createSqlField(itr.key().toString(), *itr));
    }
}

void addPreparedFieldsToRecord(QSqlRecord &record, const QList<QStringView> &columnNames)
{
    for (const auto &columnName : columnNames) {
        const auto columnNameString = columnName.toString();
        record.append(createSqlField(columnNameString, QString(QLatin1Char(':') + columnNameString)));
    }
}

QSqlField createSqlStringField(const QString &key, const QString &val)
{
    if (val.isEmpty()) {
        return createNullField(key);
    } else {
        return createSqlField(key, val);
    }
}

QSqlField createSqlField(const QString &key, const QVariant &val)
{
    QSqlField field(key, val.metaType());
    field.setValue(val);
    return field;
}

QSqlField createNullField(const QString &key)
{
    QSqlField field(key);
    field.clear();
    return field;
}

QString simpleWhereStatement(const QSqlDriver *driver, const QString &key, const QVariant &val)
{
    QSqlRecord rec;
    rec.append(createSqlField(key, val));

    return QStringLiteral(" ") + driver->sqlStatement(QSqlDriver::WhereStatement, QString(), rec, false);
}

QString simpleWhereStatement(const QSqlDriver *driver, const QMap<QString, QVariant> &keyValuePairs)
{
    QSqlRecord rec;

    const auto keys = keyValuePairs.keys();
    for (const QString &key : keys)
        rec.append(createSqlField(key, keyValuePairs.value(key)));

    return QStringLiteral(" ") + driver->sqlStatement(QSqlDriver::WhereStatement, {}, rec, false);
}

QVariant serialize(const std::optional<QDateTime> &dateTime)
{
    return dateTime ? dateTime->toMSecsSinceEpoch() : QVariant(QMetaType(QMetaType::LongLong));
}

QVariant serialize(const QDateTime &dateTime)
{
    return dateTime.isValid() ? dateTime.toMSecsSinceEpoch() : QVariant(0LL);
}

std::optional<QDateTime> parseOptDateTime(QSqlQuery &query, int index)
{
    if (auto value = query.value(index); !value.isNull()) {
        Q_ASSERT(value.metaType().id() == QMetaType::LongLong);
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
