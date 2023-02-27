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
