// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "Enums.h"

#include <QDomElement>

namespace XmlUtils
{
namespace Writer
{
template<typename T, ENABLE_IF(!QtPrivate::IsQEnumHelper<T>::Value && !std::is_enum_v<T> && !std::is_integral_v<T>)>
void text(QXmlStreamWriter *writer, const QString &key, const T &value);

template<>
void text(QXmlStreamWriter *writer, const QString &key, const QString &value)
{
    writer->writeTextElement(key, value);
}

template<typename E, ENABLE_IF(QtPrivate::IsQEnumHelper<E>::Value || std::is_enum_v<E>)>
void text(QXmlStreamWriter *writer, const QString &key, const E &value)
{
    writer->writeTextElement(key, QString::number(Enums::toIntegral(value)));
}

template<typename T, ENABLE_IF(std::is_integral_v<T>)>
void text(QXmlStreamWriter *writer, const QString &key, const T &value)
{
    writer->writeTextElement(key, QString::number(value));
}

template<typename T>
void text(QXmlStreamWriter *writer, const QString &key, const std::optional<T> &value)
{
    if (value) {
        text(writer, key, *value);
    }
}
} // namespace Writer

namespace Reader
{
template<typename T>
bool toIntegral(const QString &value, T &out)
{
    bool ok;

    if constexpr (std::is_signed_v<T>) {
        const qint64 convert = value.toLongLong(&ok);

        if (ok) {
            out = static_cast<T>(convert);
        }
    } else {
        const quint64 convert = value.toULongLong(&ok);

        if (ok) {
            out = static_cast<T>(convert);
        }
    }

    return ok;
}

template<typename T, ENABLE_IF(!QtPrivate::IsQEnumHelper<T>::Value && !std::is_enum_v<T> && !std::is_integral_v<T>)>
bool text(const QString &value, T &out);

template<>
bool text(const QString &value, QString &out)
{
    out = value;
    return true;
}

template<typename E, ENABLE_IF(QtPrivate::IsQEnumHelper<E>::Value || std::is_enum_v<E>)>
bool text(const QString &value, E &out)
{
    if constexpr (has_enum_type<E>::value) {
        using UT = E::Int;
        UT raw;

        if (toIntegral<UT>(value, raw)) {
            out = static_cast<E>(raw);
            return true;
        }

        return false;
    } else {
        using UT = std::underlying_type_t<E>;
        UT raw;

        if (toIntegral<UT>(value, raw)) {
            out = static_cast<E>(raw);
            return true;
        }

        return false;
    }
}

template<typename T, ENABLE_IF(std::is_integral_v<T>)>
bool text(const QString &value, T &out)
{
    return toIntegral<T>(value, out);
}

template<typename T>
bool text(const QString &value, std::optional<T> &out)
{
    if (value.isEmpty()) {
        return true;
    }

    T raw;

    if (text(value, raw)) {
        out = raw;
        return true;
    }

    return false;
}

template<typename T>
bool text(const QDomElement &element, std::optional<T> &out)
{
    if (element.isNull()) {
        return true;
    }

    return text(element.text(), out);
}
} // namespace Reader
} // namespace XmlUtils
