/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
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

#include <algorithm>
#include <optional>
#include <unordered_map>

#include <QVector>

template<typename T, typename Converter>
auto transform(const T &input, Converter convert)
{
	using Output = std::decay_t<decltype(convert(*input.begin()))>;
	QVector<Output> output;
	output.reserve(input.size());
	std::transform(input.begin(), input.end(), std::back_inserter(output), std::move(convert));
	return output;
}

template<typename TargetContainer, typename SourceContainer, typename Converter>
TargetContainer transform(const SourceContainer &input, Converter convert)
{
	TargetContainer output;
	output.reserve(input.size());
	std::transform(input.begin(), input.end(), std::back_inserter(output), std::move(convert));
	return output;
}

template<typename TargetContainer, typename SourceContainer, typename Converter>
TargetContainer transformNoReserve(const SourceContainer &input, Converter convert)
{
	TargetContainer output;
	std::transform(input.begin(), input.end(), std::back_inserter(output), std::move(convert));
	return output;
}

template<typename T, typename Converter>
auto transformMap(const T &input, Converter convert)
{
	using Key = std::decay_t<decltype(convert(input.front()).first)>;
	using Value = std::decay_t<decltype(convert(input.front()).second)>;
	std::unordered_map<Key, Value> hash;
	for (const auto &item : input) {
		auto [key, value] = convert(item);
		hash.insert_or_assign(std::move(key), std::move(value));
	}
	return hash;
}

template<typename T, typename Condition>
auto filter(T &&input, Condition condition)
{
	auto it = std::remove_if(input.begin(), input.end(), [condition = std::move(condition)](const auto &item) {
		return !condition(item);
	});

	input.erase(it, input.end());

	return input;
}

template<typename T, typename ConditionalConverter>
auto transformFilter(const T &input, ConditionalConverter conditionalConverter)
{
	using Output = typename std::decay_t<decltype(conditionalConverter(input.front()))>::value_type;
	QVector<Output> output;

	for (const auto &item : input) {
		if (auto result = conditionalConverter(item)) {
			output.push_back(std::move(*result));
		}
	}

	return output;
}

template <typename T, typename Func>
auto andThen(std::optional<T> &&option, Func func) -> std::invoke_result_t<Func, T> {
	if (option) {
		return func(*option);
	} else {
		return {};
	}
}

template <typename T, typename Func>
auto sum(const QVector<T> &input)
{
	using Output = std::decay_t<decltype(convert(input.front()))>;
	Output output;

	for (const auto &list : input) {
		output += list;
	}

	return output;
}

template<typename T, typename Converter>
auto transformSum(const T &input, Converter convert)
{
	using Output = std::decay_t<decltype(convert(*input.begin()))>;
	Output output;

	for (const auto &item : input) {
		output += convert(item);
	}

	return output;
}

template<typename T, typename V>
bool contains(const T &input, const T &value)
{
	return std::find(input.begin(), input.end(), value) != input.end();
}

template<typename T, typename Condition>
bool containsIf(const T &input, Condition condition)
{
	return std::find_if(input.begin(), input.end(), condition) != input.end();
}
