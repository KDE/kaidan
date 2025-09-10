// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QFutureWatcher>
// QXmpp
#include <QXmppPromise.h>
#include <QXmppTask.h>

template<typename ValueType>
auto qFutureValueType(QFuture<ValueType>) -> ValueType;
template<typename Future>
using QFutureValueType = decltype(qFutureValueType(Future()));

template<typename T>
QFuture<T> makeReadyFuture(T &&value)
{
    QPromise<T> promise;
    promise.start();
    promise.addResult(std::move(value));
    promise.finish();
    return promise.future();
}

template<typename T>
QXmppTask<T> makeReadyTask(T &&value)
{
    QXmppPromise<T> promise;
    promise.finish(std::move(value));
    return promise.task();
}

inline QXmppTask<void> makeReadyTask()
{
    QXmppPromise<void> promise;
    promise.finish();
    return promise.task();
}

template<typename T>
void reportFinishedResult(QPromise<T> &promise, const T &result)
{
    promise.addResult(result);
    promise.finish();
}

// Runs a function on targetObject's thread and returns the result via QFuture.
template<typename Function>
auto runAsync(QObject *targetObject, Function function)
{
    using ValueType = std::invoke_result_t<Function>;

    auto promise = std::make_shared<QPromise<ValueType>>();
    QMetaObject::invokeMethod(targetObject, [promise, function = std::move(function)]() mutable {
        promise->start();
        if constexpr (std::is_same_v<ValueType, void>) {
            function();
        }
        if constexpr (!std::is_same_v<ValueType, void>) {
            promise->addResult(function());
        }
        promise->finish();
    });
    return promise->future();
}

// Runs a function on targetObject's thread and reports the result on callerObject's thread.
// This is useful / required because QXmppTasks are not thread-safe.
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

// Runs a function on an object's thread (alias for QMetaObject::invokeMethod())
template<typename Function>
auto runOnThread(QObject *targetObject, Function function)
{
    QMetaObject::invokeMethod(targetObject, std::move(function));
}

// Runs a function on an object's thread and runs a callback on the caller's thread.
template<typename Function, typename Handler>
auto runOnThread(QObject *targetObject, Function function, QObject *caller, Handler handler)
{
    using ValueType = std::invoke_result_t<Function>;

    QMetaObject::invokeMethod(targetObject, [function = std::move(function), caller, handler = std::move(handler)]() mutable {
        if constexpr (std::is_same_v<ValueType, void>) {
            function();
            QMetaObject::invokeMethod(caller, [handler = std::move(handler)]() mutable {
                handler();
            });
        } else {
            auto result = function();
            QMetaObject::invokeMethod(caller, [result = std::move(result), handler = std::move(handler)]() mutable {
                handler(std::move(result));
            });
        }
    });
}

// Calls a function returning a QXmppTask on a remote thread and handles the result on the caller's
// thread
template<typename Function, typename Handler>
auto callRemoteTask(QObject *target, Function function, QObject *caller, Handler handler)
{
    QMetaObject::invokeMethod(target, [f = std::move(function), caller, h = std::move(handler)]() mutable {
        auto [task, taskContext] = f();
        task.then(taskContext, [caller, h = std::move(h)](auto r) mutable {
            QMetaObject::invokeMethod(caller, [h = std::move(h), r = std::move(r)]() mutable {
                h(std::move(r));
            });
        });
    });
}

// Calls a function returning a QXmppTask on a remote thread and handles its finishing on the
// caller's thread
template<typename Function, typename Handler>
auto callVoidRemoteTask(QObject *target, Function function, QObject *caller, Handler handler)
{
    QMetaObject::invokeMethod(target, [f = std::move(function), caller, h = std::move(handler)]() mutable {
        auto [task, taskContext] = f();
        task.then(taskContext, [caller, h = std::move(h)]() mutable {
            QMetaObject::invokeMethod(caller, [h = std::move(h)]() mutable {
                h();
            });
        });
    });
}

// Creates a future with the results from all given futures (preserving order).
template<typename T>
QFuture<QList<T>> join(QObject *context, const QList<QFuture<T>> &futures)
{
    if (futures.empty()) {
        return makeReadyFuture<QList<T>>({});
    }

    struct State {
        QPromise<QList<T>> promise;
        QObject *context = nullptr;
        QList<T> results;
        QList<QFuture<T>> futures;
        qsizetype i = 0;
        // keep-alive during awaits()
        std::shared_ptr<State> self;

        void joinOne()
        {
            if (i < futures.size()) {
                futures[i].then(context, [this](T &&value) {
                    results.push_back(std::move(value));
                    i++;
                    joinOne();
                });
            } else {
                promise.addResult(results);
                promise.finish();
                // release
                self.reset();
            }
        }
    };

    auto state = std::make_shared<State>();
    state->context = context;
    state->results.reserve(futures.size());
    state->futures = futures;
    state->self = state;

    auto future = state->promise.future();
    state->joinOne();
    return future;
}

// Creates a future for multiple futures without a return value.
template<typename T>
QFuture<void> joinVoidFutures(QObject *context, QList<QFuture<T>> &&futures)
{
    int futureCount = futures.size();
    auto finishedFutureCount = std::make_shared<int>();

    auto promise = std::make_shared<QPromise<void>>();

    for (auto future : futures) {
        future.then(context, [=]() mutable {
            if (++(*finishedFutureCount) == futureCount) {
                promise->finish();
            }
        });
    }

    return promise->future();
}
