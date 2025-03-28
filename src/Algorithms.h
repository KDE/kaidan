// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tobias Berner <tcberner@freebsd.org>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <algorithm>
#include <optional>
#include <unordered_map>
#if __has_include(<__cpp_lib_ranges_enumerate>)
#include <ranges>
#endif
// Qt
#include <QList>

template<typename T, typename Converter>
auto transform(const T &input, Converter convert)
{
    using Output = std::decay_t<decltype(convert(*input.begin()))>;
    QList<Output> output;
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
    QList<Output> output;

    for (const auto &item : input) {
        if (auto result = conditionalConverter(item)) {
            output.push_back(std::move(*result));
        }
    }

    return output;
}

template<typename TargetContainer, typename SourceContainer, typename ConditionalConverter>
auto transformFilter(const SourceContainer &input, ConditionalConverter conditionalConverter)
{
    TargetContainer output;

    for (const auto &item : input) {
        if (auto result = conditionalConverter(item)) {
            output.push_back(std::move(*result));
        }
    }

    return output;
}

template<typename T, typename Func>
auto andThen(std::optional<T> &&option, Func func) -> std::invoke_result_t<Func, T>
{
    if (option) {
        return func(*option);
    } else {
        return {};
    }
}

template<typename T, typename Func>
auto sum(const QList<T> &input)
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

/**
 * Removes duplicates from a container.
 */
template<typename Container>
static void makeUnique(Container &container)
{
    std::sort(container.begin(), container.end());
    container.erase(std::unique(container.begin(), container.end()), container.end());
}

/**
 * Returns whether two containers have at least one element in common.
 */
template<typename Container>
static bool containCommonElement(Container &containerA, Container &containerB)
{
    return std::search(containerA.cbegin(), containerA.cend(), containerB.cbegin(), containerB.cend()) != containerA.cend();
}

/**
 * Runs a function on all parts of a text.
 *
 * @param text text that is split into parts
 * @param isSeparator function returning whether a character passed to it should split the text
 * @param processPart function run on each text part
 */
template<typename IsSeparator, typename ProcessPart>
auto processTextParts(QStringView text, IsSeparator isSeparator, ProcessPart processPart)
{
    qsizetype start = 0;

#if __has_include(<__cpp_lib_ranges_enumerate>)
    for (auto [i, character] : std::views::enumerate(text)) {
        if (const auto atEnd = i == text.size() - 1; isSeparator(character) || atEnd) {
            auto end = atEnd ? text.size() : i;
            auto part = QStringView(text).mid(start, end - start);

            processPart(start, part);

            start = i + 1;
        }
    }
#else
    for (auto i = 0; i < text.size(); i++) {
        if (const auto atEnd = i == text.size() - 1; isSeparator(text.at(i)) || atEnd) {
            auto end = atEnd ? text.size() : i;
            auto part = QStringView(text).mid(start, end - start);

            processPart(start, part);

            start = i + 1;
        }
    }
#endif
}
