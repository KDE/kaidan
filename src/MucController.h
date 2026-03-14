// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// QXmpp
#include "QXmppPepBookmarkManager.h"

// QXmpp
class QXmppMucBookmark;
class QXmppMucManagerV2;
// Kaidan
class AccountSettings;
class GroupChatController;
class MessageController;
struct GroupChatService;
struct RosterItem;

/**
 * This class provides XEP-0369: Mediated Information eXchange (MIX) functionality.
 */
class MucController : public QObject
{
    Q_OBJECT

public:
    explicit MucController(AccountSettings *accountSettings,
                           GroupChatController *groupChatController,
                           MessageController *messageController,
                           QXmppMucManagerV2 *mucManager,
                           QXmppPepBookmarkManager *bookmarkManager,
                           QObject *parent = nullptr);

private:
    void replaceBookmarks();
    void addBookmarks(const QList<QXmppMucBookmark> &bookmarks);
    void updateBookmarks(const QList<QXmppPepBookmarkManager::BookmarkChange> &bookmarkUpdates);
    void removeBookmarks(const QList<QString> &bookmarkJids);

    QList<RosterItem> createChats(const QString &accountJid, const QList<QXmppMucBookmark> &bookmarks) const;

    AccountSettings *const m_accountSettings;

    GroupChatController *const m_groupChatController;
    MessageController *const m_messageController;

    QXmppPepBookmarkManager *const m_bookmarkManager;
    QXmppMucManagerV2 *const m_mucManager;
};
