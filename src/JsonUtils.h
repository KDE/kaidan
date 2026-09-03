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
void addValue(QJsonObject &object, QStringView key, const T &newValue);

template<>
void addValue(QJsonObject &object, QStringView key, const QString &newValue)
{
    Q_ASSERT(!key.isEmpty());

    if (!newValue.isEmpty()) {
        object.insert(key, newValue);
    }
}

template<>
void addValue(QJsonObject &object, QStringView key, const int &newValue)
{
    Q_ASSERT(!key.isEmpty());
    object.insert(key, newValue);
}

template<>
void addValue(QJsonObject &object, QStringView key, const bool &newValue)
{
    Q_ASSERT(!key.isEmpty());
    object.insert(key, newValue);
}

template<>
void addValue(QJsonObject &object, QStringView key, const QStringList &newValue)
{
    Q_ASSERT(!key.isEmpty());

    if (!newValue.isEmpty()) {
        object.insert(key, newValue.join(QLatin1Char(',')));
    }
}
} // namespace Json

#define JSON_VALUE(TYPE, KEY) Json::value<TYPE>(object, KEY)
#define ADD_JSON_VALUE(KEY, VALUE) Json::addValue(object, KEY, VALUE)
