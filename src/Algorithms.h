// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tobias Berner <tcberner@freebsd.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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

	return std::move(input);
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
