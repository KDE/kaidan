// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
// QXmpp
#include <QXmppPromise.h>
#include <QXmppTask.h>
// Kaidan
#include "Algorithms.h"

template<typename T>
void reportFinishedResult(QPromise<T> &promise, const T &result)
{
    promise.addResult(result);
    promise.finish();
}

// Runs a function on targetObject's thread and reports the result via QFuture.
template<typename Function>
auto runAsync(QObject *targetObject, Function function)
{
    using ValueType = std::invoke_result_t<Function>;

    auto promise = std::make_shared<QPromise<ValueType>>();

    QMetaObject::invokeMethod(targetObject, [promise, function = std::move(function)]() mutable {
        promise->start();

        if constexpr (std::is_same_v<ValueType, void>) {
            function();
        } else {
            promise->addResult(function());
        }

        promise->finish();
    });

    return promise->future();
}

// Runs a function on targetObject's thread and reports the result via QXmppTask on callerObject's thread.
// That is required because QXmppTask is not thread-safe.
template<typename Function>
auto runAsyncTask(QObject *callerObject, QObject *targetObject, Function function)
{
    using ValueType = std::invoke_result_t<Function>;

    QXmppPromise<ValueType> promise;
    auto task = promise.task();

    QMetaObject::invokeMethod(targetObject, [callerObject, promise = std::move(promise), function = std::move(function)]() mutable {
        if constexpr (std::is_same_v<ValueType, void>) {
            function();
            QMetaObject::invokeMethod(callerObject, [promise = std::move(promise)]() mutable {
                promise.finish();
            });
        } else {
            auto value = function();
            QMetaObject::invokeMethod(callerObject, [promise = std::move(promise), value = std::move(value)]() mutable {
                promise.finish(std::move(value));
            });
        }
    });

    return task;
}

// Creates a future with the results from all given futures (preserving order).
template<typename T>
QFuture<QList<T>> join(QList<QFuture<T>> &&futures)
{
    Q_ASSERT(!futures.isEmpty());

    return QtFuture::whenAll(futures.begin(), futures.end()).then([](const QList<QFuture<T>> &futures) {
        return transform(futures, [](const QFuture<T> &future) {
            return future.result();
        });
    });
}

// Creates a future for multiple futures without a return value.
template<typename T>
QFuture<void> joinVoidFutures(QList<QFuture<T>> &&futures)
{
    return QtFuture::whenAll(futures.begin(), futures.end()).then([](const QList<QFuture<T>> &futures) {
        Q_UNUSED(futures)
        return;
    });
}
