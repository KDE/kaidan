// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatStateController.h"

// Qt
#include <QTimer>
// Kaidan
#include "Account.h"
#include "ChatController.h"
#include "MessageController.h"

using namespace std::chrono_literals;

constexpr auto TYPING_TIMEOUT = 2s;
constexpr auto PAUSED_TIMEOUT = 10s;

ChatStateController::ChatStateController(Connection *connection,
                                         PresenceCache *presenceCache,
                                         ChatController *chatController,
                                         MessageController *messageController,
                                         QObject *parent)
    : QObject(parent)
    , m_connection(connection)
    , m_chatController(chatController)
    , m_messageController(messageController)
    , m_composingTimer(new QTimer(this))
    , m_pausedTimer(new QTimer(this))
{
    m_contactResourcesWatcher.setJid(m_chatController->jid());
    m_contactResourcesWatcher.setPresenceCache(presenceCache);

    addChatStateResetLogoutTask();

    connect(m_connection, &Connection::connected, this, &ChatStateController::addChatStateResetLogoutTaskOnConnected);
    connect(m_chatController, &ChatController::rosterItemChanged, this, &ChatStateController::resetChatStateOnDisablingSetting);

    m_pausedTimer->setSingleShot(true);
    m_pausedTimer->setInterval(PAUSED_TIMEOUT);

    m_composingTimer->setSingleShot(true);
    m_composingTimer->setInterval(TYPING_TIMEOUT);
    m_composingTimer->callOnTimeout(this, [this] {
        sendChatStateIfEnabled(QXmppMessage::State::Paused);
        m_pausedTimer->start();
    });
}

void ChatStateController::setComposing()
{
    sendChatStateIfEnabled(QXmppMessage::Composing);
}

void ChatStateController::resetChatState()
{
    if (m_chatState == QXmppMessage::State::Composing || m_chatState == QXmppMessage::State::Paused) {
        sendChatStateIfEnabled(QXmppMessage::State::Inactive);
    }
}

void ChatStateController::resetPreviousChat()
{
    if (m_chatStateResetPromise) {
        if (m_chatStateResetPromise->future().isStarted()) {
            m_chatStateResetPromise->finish();
        } else {
            m_connection->removeLogoutTask(m_chatStateResetPromise);
        }

        m_chatStateResetPromise.reset();
    }

    m_composingTimer->stop();
    m_pausedTimer->stop();

    resetChatState();
}

void ChatStateController::sendChatStateIfEnabled(QXmppMessage::State chatState)
{
    if (m_chatController->rosterItem().chatStateSendingEnabled) {
        sendChatState(chatState);
    } else {
        m_chatState = chatState;
    }
}

void ChatStateController::sendChatState(QXmppMessage::State chatState)
{
    if (chatState == m_chatState) {
        return;
    }

    m_chatState = chatState;

    if (!m_contactResourcesWatcher.resourcesCount()) {
        return;
    }

    if (chatState == QXmppMessage::State::Composing) {
        m_composingTimer->start();
    } else {
        m_composingTimer->stop();
    }

    const auto encryption = m_chatController->activeEncryption();

    if (const auto isGroupChat = m_chatController->rosterItem().isGroupChat()) {
        m_messageController->sendChatState(m_chatController->jid(), isGroupChat, chatState, encryption, m_chatController->groupChatUserJids());
    } else {
        m_messageController->sendChatState(m_chatController->jid(), isGroupChat, chatState, encryption);
    }
}

void ChatStateController::addChatStateResetLogoutTask()
{
    m_chatStateResetPromise = m_connection->addLogoutTask([this]() {
        resetChatState();

        m_chatStateResetPromise->finish();
        m_chatStateResetPromise.reset();
    });
}

void ChatStateController::addChatStateResetLogoutTaskOnConnected()
{
    if (!m_chatStateResetPromise) {
        addChatStateResetLogoutTask();
    }
}

void ChatStateController::resetChatStateOnDisablingSetting()
{
    if (!m_chatController->rosterItem().chatStateSendingEnabled
        && (m_chatState == QXmppMessage::State::Composing || m_chatState == QXmppMessage::State::Paused)) {
        sendChatState(QXmppMessage::State::Inactive);
    }
}

#include "moc_ChatStateController.cpp"
