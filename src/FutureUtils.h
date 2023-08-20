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
#include <QXmppTask.h>
#include <QXmppPromise.h>

template<typename ValueType>
auto qFutureValueType(QFuture<ValueType>) -> ValueType;
template<typename Future>
using QFutureValueType = decltype(qFutureValueType(Future()));

template<typename T>
QFuture<T> makeReadyFuture(T &&value)
{
	QFutureInterface<T> interface(QFutureInterfaceBase::Started);
	interface.reportResult(std::move(value));
	interface.reportFinished();
	return interface.future();
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

template<typename T, typename Handler>
void await(const QFuture<T> &future, QObject *context, Handler handler)
{
	auto *watcher = new QFutureWatcher<T>(context);
	QObject::connect(watcher, &QFutureWatcherBase::finished,
	                 context, [watcher, handler = std::move(handler)]() mutable {
		if constexpr (std::is_same_v<T, void> || std::is_invocable<Handler>::value) {
			handler();
		}
		if constexpr (!std::is_same_v<T, void> && !std::is_invocable<Handler>::value) {
			handler(watcher->result());
		}
		watcher->deleteLater();
	});
	watcher->setFuture(future);
}

template<typename Runner, typename Functor, typename Handler>
void await(Runner *runner, Functor function, QObject *context, Handler handler)
{
	// function() returns QFuture<T>, we get T by using QFutureValueType
	using Result = QFutureValueType<std::invoke_result_t<Functor>>;

	auto *watcher = new QFutureWatcher<Result>(context);
	QObject::connect(watcher, &QFutureWatcherBase::finished,
	                 context, [watcher, handler = std::move(handler)]() mutable {
		if constexpr (std::is_same_v<Result, void> || std::is_invocable<Handler>::value) {
			handler();
		}
		if constexpr (!std::is_same_v<Result, void> && !std::is_invocable<Handler>::value) {
			handler(watcher->result());
		}
		watcher->deleteLater();
	});

	QMetaObject::invokeMethod(runner, [watcher, function = std::move(function)]() mutable {
		watcher->setFuture(function());
	});
}

template<typename T>
void reportFinishedResult(QFutureInterface<T> &interface, const T &result)
{
	interface.reportResult(result);
	interface.reportFinished();
}

// Runs a function on targetObject's thread and returns the result via QFuture.
template<typename Function>
auto runAsync(QObject *targetObject, Function function)
{
	using ValueType = std::invoke_result_t<Function>;

	QFutureInterface<ValueType> interface;
	QMetaObject::invokeMethod(targetObject, [interface, function = std::move(function)]() mutable {
		interface.reportStarted();
		if constexpr (std::is_same_v<ValueType, void>) {
			function();
		}
		if constexpr (!std::is_same_v<ValueType, void>) {
			interface.reportResult(function());
		}
		interface.reportFinished();
	});
	return interface.future();
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
			QMetaObject::invokeMethod(
						caller,
						[handler = std::move(handler)]() mutable {
				handler();
			});
		} else {
			auto result = function();
			QMetaObject::invokeMethod(
						caller,
						[result = std::move(result), handler = std::move(handler)]() mutable {
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
			QMetaObject::invokeMethod(caller,
				[h = std::move(h), r = std::move(r)]() mutable { h(std::move(r)); });
		});
	});
}

// Creates a future with the results from all given futures.
template <typename T>
QFuture<QVector<T>> join(QObject *context, QVector<QFuture<T>> &&futures)
{
	auto results = std::make_shared<QVector<T>>();
	results->reserve(futures.size());

	int futureCount = futures.size();

	QFutureInterface<QVector<T>> interface;

	for (auto future : futures) {
		await(future, context, [=](auto result) mutable {
			results->push_back(result);
			if (results->size() == futureCount) {
				interface.reportResult(*results);
				interface.reportFinished();
			}
		});
	}

	return interface.future();
}
