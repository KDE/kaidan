// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterItemWatcher.h"
#ifndef UNIT_TEST
#include "RosterModel.h"
#endif

RosterItemNotifier &RosterItemNotifier::instance()
{
	static RosterItemNotifier notifier;
	return notifier;
}

void RosterItemNotifier::notifyWatchers(const QString &jid, const std::optional<RosterItem> &item)
{
	auto [keyBegin, keyEnd] = m_itemWatchers.equal_range(jid);
	std::for_each(keyBegin, keyEnd, [&](const auto &pair) {
		pair.second->notify(item);
	});
}

void RosterItemNotifier::registerItemWatcher(const QString &jid, RosterItemWatcher *watcher)
{
	m_itemWatchers.emplace(jid, watcher);
}

void RosterItemNotifier::unregisterItemWatcher(const QString &jid, RosterItemWatcher *watcher)
{
	auto [keyBegin, keyEnd] = m_itemWatchers.equal_range(jid);
	auto itr = std::find_if(keyBegin, keyEnd, [watcher](const auto &pair) {
		return pair.second == watcher;
	});
	if (itr != keyEnd) {
		m_itemWatchers.erase(itr);
	}
}

RosterItemWatcher::RosterItemWatcher(QObject *parent)
	: QObject(parent)
{
}

RosterItemWatcher::~RosterItemWatcher()
{
	unregister();
}

const QString &RosterItemWatcher::jid() const
{
	return m_jid;
}

void RosterItemWatcher::setJid(const QString &jid)
{
	if (jid != m_jid) {
		unregister();
		m_jid = jid;
		RosterItemNotifier::instance().registerItemWatcher(m_jid, this);
		emit jidChanged();
#ifndef UNIT_TEST
		notify(RosterModel::instance()->findItem(m_jid));
#endif
	}
}

const RosterItem &RosterItemWatcher::item() const
{
	return m_item;
}

void RosterItemWatcher::unregister()
{
	if (!m_jid.isNull()) {
		RosterItemNotifier::instance().unregisterItemWatcher(m_jid, this);
	}
}

void RosterItemWatcher::notify(const std::optional<RosterItem> &item)
{
	if (item) {
		m_item = *item;
	} else {
		m_item = {};
	}
	emit itemChanged();
}
