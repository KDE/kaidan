// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QPromise>
// QXmpp
#include <QXmppMessage.h>
// Kaidan
#include "PresenceCache.h"

// Qt
class QTimer;
// Kaidan
class ChatController;
class Connection;
class MessageController;

class ChatStateController : public QObject
{
    Q_OBJECT

public:
    ChatStateController(Connection *connection,
                        PresenceCache *presenceCache,
                        ChatController *chatController,
                        MessageController *messageController,
                        QObject *parent = nullptr);

    Q_INVOKABLE void setComposing();
    Q_INVOKABLE void resetChatState();

    void resetPreviousChat();

private:
    void sendChatStateIfEnabled(QXmppMessage::State chatState);
    void sendChatState(QXmppMessage::State chatState);

    void addChatStateResetLogoutTask();
    void addChatStateResetLogoutTaskOnConnected();

    void resetChatStateOnDisablingSetting();

    Connection *const m_connection;

    ChatController *const m_chatController;
    MessageController *const m_messageController;

    UserResourcesWatcher m_contactResourcesWatcher;

    QXmppMessage::State m_chatState = QXmppMessage::State::Inactive;
    std::shared_ptr<QPromise<void>> m_chatStateResetPromise;

    QTimer *const m_composingTimer;
    QTimer *const m_pausedTimer;
};
