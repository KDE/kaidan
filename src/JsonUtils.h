// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QJsonArray>
#include <QJsonObject>

namespace Json
{
// value

template<typename T>
T value(const QJsonObject &object, QStringView key);

template<>
QString value(const QJsonObject &object, QStringView key)
{
    Q_ASSERT(!key.isEmpty());
    return object.value(key).toString();
}

template<>
int value(const QJsonObject &object, QStringView key)
{
    Q_ASSERT(!key.isEmpty());
    return static_cast<int>(object.value(key).toDouble());
}

template<>
bool value(const QJsonObject &object, QStringView key)
{
    Q_ASSERT(!key.isEmpty());
    return object.value(key).toBool();
}

// addValue

template<typename T>
void addValue(QJsonObject &object, QStringView key, const T &value);

template<>
void addValue(QJsonObject &object, QStringView key, const QString &value)
{
    Q_ASSERT(!key.isEmpty());

    if (!value.isEmpty()) {
        object.insert(key, value);
    }
}

template<>
void addValue(QJsonObject &object, QStringView key, const int &value)
{
    Q_ASSERT(!key.isEmpty());
    object.insert(key, value);
}

template<>
void addValue(QJsonObject &object, QStringView key, const bool &value)
{
    Q_ASSERT(!key.isEmpty());
    object.insert(key, value);
}

template<>
void addValue(QJsonObject &object, QStringView key, const QStringList &value)
{
    Q_ASSERT(!key.isEmpty());

    if (!value.isEmpty()) {
        object.insert(key, value.join(QLatin1Char(',')));
    }
}
} // namespace Json

#define JSON_VALUE(TYPE, KEY) Json::value<TYPE>(object, KEY)
#define ADD_JSON_VALUE(KEY, VALUE) Json::addValue(object, KEY, VALUE)
