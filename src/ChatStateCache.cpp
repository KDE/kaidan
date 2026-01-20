// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatStateCache.h"

// Kaidan
#include "Account.h"
#include "MessageController.h"

using namespace std::chrono_literals;

constexpr auto ACTIVE_TIMEOUT = 1min;

ChatStateCache::ChatStateWrapper::ChatStateWrapper(ChatStateWrapper &&other)
{
    std::swap(jid, other.jid);
    std::swap(chatState, other.chatState);
    std::swap(timer, other.timer);
}

ChatStateCache::ChatStateWrapper::~ChatStateWrapper()
{
    if (timer) {
        timer->deleteLater();
    }
}

ChatStateCache::ChatStateWrapper &ChatStateCache::ChatStateWrapper::operator=(ChatStateWrapper &&other)
{
    std::swap(jid, other.jid);
    std::swap(chatState, other.chatState);
    std::swap(timer, other.timer);

    return *this;
}

ChatStateCache::ChatStateCache(Connection *connection, MessageController *messageController, QObject *parent)
    : QObject(parent)
{
    connect(connection, &Connection::disconnected, this, &ChatStateCache::handleDisconnected);
    connect(messageController, &MessageController::chatStateReceived, this, &ChatStateCache::handleChatStateReceived);
}

QXmppMessage::State ChatStateCache::chatState(const QString &jid) const
{
    const auto itr = std::ranges::find_if(m_chatStateWrappers, [jid](const ChatStateWrapper &chatStateWrapper) {
        return chatStateWrapper.jid == jid;
    });

    if (itr != m_chatStateWrappers.cend()) {
        return itr->chatState;
    }

    return QXmppMessage::State::Inactive;
}

void ChatStateCache::handleDisconnected()
{
    m_chatStateWrappers.clear();
    Q_EMIT chatStatesChanged();
}

void ChatStateCache::handleChatStateReceived(const QString &jid, QXmppMessage::State chatState)
{
    auto itr = std::ranges::find_if(m_chatStateWrappers, [jid](const ChatStateWrapper &chatStateWrapper) {
        return chatStateWrapper.jid == jid;
    });

    if (itr == m_chatStateWrappers.end()) {
        if (chatState == QXmppMessage::State::Composing || chatState == QXmppMessage::State::Paused) {
            auto *timer = new QTimer();
            timer->setSingleShot(true);
            timer->setInterval(ACTIVE_TIMEOUT);
            timer->callOnTimeout(this, [this, jid]() {
                removeChatState(jid);
            });

            ChatStateWrapper chatStateWrapper;
            chatStateWrapper.jid = jid;
            chatStateWrapper.chatState = chatState;
            chatStateWrapper.timer = timer;

            m_chatStateWrappers.push_back(std::move(chatStateWrapper));
            timer->start();

            Q_EMIT chatStateChanged(jid);
        }
    } else if (itr->chatState != chatState) {
        if (chatState == QXmppMessage::State::Composing || chatState == QXmppMessage::State::Paused) {
            itr->chatState = chatState;
            itr->timer->start();
        } else {
            m_chatStateWrappers.erase(itr);
        }

        Q_EMIT chatStateChanged(jid);
    }
}

void ChatStateCache::removeChatState(const QString &jid)
{
    const auto itr = std::ranges::find_if(m_chatStateWrappers, [jid](const ChatStateWrapper &chatStateWrapper) {
        return chatStateWrapper.jid == jid;
    });

    if (itr != m_chatStateWrappers.cend()) {
        m_chatStateWrappers.erase(itr);
        Q_EMIT chatStateChanged(jid);
    }
}
