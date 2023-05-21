// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <optional>
#include <unordered_map>
#include <QObject>

#include "RosterItem.h"

class RosterItemWatcher;

class RosterItemNotifier
{
public:
	static RosterItemNotifier &instance();

	void notifyWatchers(const QString &jid, const std::optional<RosterItem> &item);
	void registerItemWatcher(const QString &jid, RosterItemWatcher *watcher);
	void unregisterItemWatcher(const QString &jid, RosterItemWatcher *watcher);

private:
	RosterItemNotifier() = default;

	std::unordered_multimap<QString, RosterItemWatcher *> m_itemWatchers;
};

class RosterItemWatcher : public QObject
{
	Q_OBJECT
	Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
	Q_PROPERTY(const RosterItem &item READ item NOTIFY itemChanged)
public:
	explicit RosterItemWatcher(QObject *parent = nullptr);
	~RosterItemWatcher();

	const QString &jid() const;
	void setJid(const QString &jid);
	Q_SIGNAL void jidChanged();

	const RosterItem &item() const;
	Q_SIGNAL void itemChanged();

private:
	friend class RosterItemNotifier;

	void notify(const std::optional<RosterItem> &item);
	void unregister();

	QString m_jid;
	RosterItem m_item;
	bool m_outdated = false;
};
