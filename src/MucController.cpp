// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MucController.h"

// QXmpp
#include <QXmppClient.h>
#include <QXmppMucManagerV2.h>
#include <QXmppPepBookmarkManager.h>
#include <QXmppTask.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "ChatController.h"
#include "ClientController.h"
#include "GroupChatController.h"
#include "MessageController.h"
#include "RosterDb.h"
#include "RosterModel.h"

MucController::MucController(AccountSettings *accountSettings,
                             GroupChatController *groupChatController,
                             MessageController *messageController,
                             QXmppMucManagerV2 *mucManager,
                             QXmppPepBookmarkManager *bookmarkManager,
                             QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_groupChatController(groupChatController)
    , m_messageController(messageController)
    , m_bookmarkManager(bookmarkManager)
    , m_mucManager(mucManager)
{
    connect(m_bookmarkManager, &QXmppPepBookmarkManager::bookmarksReset, this, &MucController::replaceBookmarks);
    connect(m_bookmarkManager, &QXmppPepBookmarkManager::bookmarksAdded, this, &MucController::addBookmarks);
    connect(m_bookmarkManager, &QXmppPepBookmarkManager::bookmarksChanged, this, &MucController::updateBookmarks);
    connect(m_bookmarkManager, &QXmppPepBookmarkManager::bookmarksRemoved, this, &MucController::removeBookmarks);
}

void MucController::replaceBookmarks()
{
    const auto accountJid = m_accountSettings->jid();

    if (const auto &bookmarks = m_bookmarkManager->bookmarks(); !bookmarks) {
        // Keep the stored bookmarks in case of an error response from the server.
        return;
    } else if (bookmarks->isEmpty()) {
        RosterDb::instance()->removeBookmarks(accountJid);
    } else {
        RosterDb::instance()->replaceBookmarks(accountJid, createChats(accountJid, *bookmarks));
    }
}

void MucController::addBookmarks(const QList<QXmppMucBookmark> &bookmarks)
{
    const auto accountJid = m_accountSettings->jid();
    RosterDb::instance()->addBookmarks(accountJid, createChats(accountJid, bookmarks));
}

void MucController::updateBookmarks(const QList<QXmppPepBookmarkManager::BookmarkChange> &bookmarkUpdates)
{
    const auto accountJid = m_accountSettings->jid();

    for (const auto &bookmarkUpdate : bookmarkUpdates) {
        const auto oldBookmark = bookmarkUpdate.oldBookmark;
        const auto newBookmark = bookmarkUpdate.newBookmark;

        if (const auto newName = newBookmark.name(); oldBookmark.name() != newName) {
            RosterDb::instance()->updateItem(accountJid, oldBookmark.jid(), [newName](RosterItem &item) {
                // Do not overwrite a possibly existing MIX item.
                // That is the case if a group chat is joined via MIX by one client and via MUC by another one.
                if (item.origin == RosterItem::Origin::Bookmarks) {
                    item.name = newName;
                }
            });
        }
    }
}

void MucController::removeBookmarks(const QList<QString> &bookmarkJids)
{
    RosterDb::instance()->removeBookmarks(m_accountSettings->jid(), bookmarkJids);
}

QList<RosterItem> MucController::createChats(const QString &accountJid, const QList<QXmppMucBookmark> &bookmarks) const
{
    QList<RosterItem> chats;

    for (const auto &bookmark : bookmarks) {
        // Do not overwrite a possibly existing MIX item.
        // That is the case if a group chat is joined via MIX by one client and via MUC by another one.
        if (const auto chatJid = bookmark.jid(); !RosterModel::instance()->hasItem(accountJid, chatJid)) {
            RosterItem chat;

            chat.accountJid = accountJid;
            chat.jid = chatJid;
            chat.origin = RosterItem::Origin::Bookmarks;
            chat.name = bookmark.name();

            chats.append(chat);
        }
    }

    return chats;
}

#include "moc_MucController.cpp"
