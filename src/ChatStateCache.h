// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QTimer>
// QXmpp
#include <QXmppMessage.h>

// Kaidan
class Connection;
class MessageController;

class ChatStateCache : public QObject
{
    Q_OBJECT

public:
    struct ChatStateWrapper {
        ChatStateWrapper() = default;
        ChatStateWrapper(ChatStateWrapper &&other);
        ~ChatStateWrapper();

        ChatStateWrapper &operator=(ChatStateWrapper &&other);

        QString jid;
        QXmppMessage::State chatState;
        // Timer to reset the chat partner's state if their client did not send a new state for too long.
        // That is especially useful if the chat partner's connection is lost while a state other than inactive was set.
        QTimer *timer = nullptr;

        Q_DISABLE_COPY(ChatStateWrapper)
    };

    ChatStateCache(Connection *connection, MessageController *messageController, QObject *parent = nullptr);

    QXmppMessage::State chatState(const QString &jid) const;
    Q_SIGNAL void chatStateChanged(const QString &jid);
    Q_SIGNAL void chatStatesChanged();

private:
    void handleDisconnected();
    void handleChatStateReceived(const QString &jid, QXmppMessage::State chatState);
    void removeChatState(const QString &jid);

    std::vector<ChatStateWrapper> m_chatStateWrappers;
};
