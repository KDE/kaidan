// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <type_traits>
// Qt
#include <QJSEngine>

/**
 * Returns the existing instance of T from its QML_SINGLETON create() function.
 */
template<typename T>
T *qmlSingletonInstance()
{
    static_assert(!std::is_default_constructible_v<T>, "QML only calls create() if the type is not default-constructible");

    auto *instance = T::instance();
    Q_ASSERT(instance);
    QJSEngine::setObjectOwnership(instance, QJSEngine::CppOwnership);
    return instance;
}
