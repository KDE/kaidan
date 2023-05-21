// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <unordered_map>
#include <optional>

#include <QString>

template<typename Key, typename Value>
class AbstractWatcher;

template<typename Key, typename Value>
class AbstractNotifier
{
	using Notifier = AbstractNotifier<Key, Value>;
	using Watcher = AbstractWatcher<Key, Value>;
public:
	static Notifier &instance()
	{
		static Notifier notifier;
		return notifier;
	}
	void notifyWatchers(const Key &key, const Value &item)
	{
		auto [keyBegin, keyEnd] = m_itemWatchers.equal_range(key);
		std::for_each(keyBegin, keyEnd, [&](const auto &pair) {
			pair.second->notify(item);
		});
	}
	void registerWatcher(const Key &key, Watcher *watcher)
	{
		m_itemWatchers.emplace(key, watcher);
	}
	void unregisterWatcher(const Key &key, Watcher *watcher)
	{
		auto [keyBegin, keyEnd] = m_itemWatchers.equal_range(key);
		auto itr = std::find_if(keyBegin, keyEnd, [watcher](const auto &pair) {
			return pair.second == watcher;
		});
		if (itr != keyEnd) {
			m_itemWatchers.erase(itr);
		}
	}

private:
	std::unordered_multimap<Key, AbstractWatcher<Key, Value> *> m_itemWatchers;
};

template<typename Key, typename Value>
class AbstractWatcher
{
protected:
	using Notifier = AbstractNotifier<Key, Value>;
	friend class AbstractNotifier<Key, Value>;

	virtual ~AbstractWatcher()
	{
		unregisterWatcher();
	}

	virtual void notify(const Value &value) = 0;
	void setKey(Key &&key)
	{
		unregisterWatcher();
		m_key = std::move(key);
		registerWatcher();
	}
	void registerWatcher()
	{
		Notifier::instance().registerWatcher(*m_key, this);
	}
	void unregisterWatcher()
	{
		if (m_key) {
			Notifier::instance().unregisterWatcher(m_key.value(), this);
		}
	}

	std::optional<Key> m_key;
};

