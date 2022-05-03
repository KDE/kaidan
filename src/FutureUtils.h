/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
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

#pragma once

#include <QFuture>
#include <QFutureWatcher>

template<typename ValueType>
auto qFutureValueType(QFuture<ValueType>) -> ValueType;
template<typename Future>
using QFutureValueType = decltype(qFutureValueType(Future()));

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
