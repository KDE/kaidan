// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QObject>
// Kaidan
#include "RosterItem.h"

class RosterItemWatcher;

class RosterItemNotifier
{
public:
    static RosterItemNotifier &instance();

    void notifyWatchers(const QString &accountJid, const QString &jid, const std::optional<RosterItem> &item);
    void registerItemWatcher(RosterItemWatcher *watcher);
    void unregisterItemWatcher(RosterItemWatcher *watcher);

private:
    RosterItemNotifier() = default;

    QList<RosterItemWatcher *> m_itemWatchers;
};

class RosterItemWatcher : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
    Q_PROPERTY(QString jid READ jid WRITE setJid NOTIFY jidChanged)
    Q_PROPERTY(const RosterItem &item READ item NOTIFY itemChanged)
public:
    explicit RosterItemWatcher(QObject *parent = nullptr);
    ~RosterItemWatcher();

    const QString &accountJid() const;
    void setAccountJid(const QString &accountJid);
    Q_SIGNAL void accountJidChanged();

    const QString &jid() const;
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    const RosterItem &item() const;
    Q_SIGNAL void itemChanged();

private:
    friend class RosterItemNotifier;

    void registerIfComplete();
    void unregister();
    void notify(const std::optional<RosterItem> &item);

    QString m_accountJid;
    QString m_jid;
    RosterItem m_item;
    bool m_outdated = false;
};
