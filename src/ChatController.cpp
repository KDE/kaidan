// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatController.h"

#include <chrono>

// Qt
#include <QGuiApplication>
#include <QTimer>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "EncryptionController.h"
#include "EncryptionWatcher.h"
#include "MessageController.h"
#include "MessageModel.h"
#include "RosterItemWatcher.h"
#include "RosterModel.h"

using namespace std::chrono_literals;

constexpr auto PAUSED_TYPING_TIMEOUT = 10s;
constexpr auto ACTIVE_TIMEOUT = 2min;
constexpr auto TYPING_TIMEOUT = 2s;

ChatController *ChatController::s_instance = nullptr;

ChatController *ChatController::instance()
{
    return s_instance;
}

ChatController::ChatController(QObject *parent)
    : QObject(parent)
    , m_accountEncryptionWatcher(new EncryptionWatcher(this))
    , m_chatEncryptionWatcher(new EncryptionWatcher(this))
    , m_composingTimer(new QTimer(this))
    , m_stateTimeoutTimer(new QTimer(this))
    , m_inactiveTimer(new QTimer(this))
    , m_chatPartnerChatStateTimeout(new QTimer(this))
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(&m_rosterItemWatcher, &RosterItemWatcher::itemChanged, this, &ChatController::rosterItemChanged);
    connect(&m_rosterItemWatcher, &RosterItemWatcher::itemChanged, this, &ChatController::isEncryptionEnabledChanged);

    connect(this, &ChatController::encryptionChanged, this, &ChatController::isEncryptionEnabledChanged);

    connect(m_accountEncryptionWatcher, &EncryptionWatcher::hasUsableDevicesChanged, this, &ChatController::isEncryptionEnabledChanged);
    connect(m_chatEncryptionWatcher, &EncryptionWatcher::hasUsableDevicesChanged, this, &ChatController::isEncryptionEnabledChanged);

    // Timer to set state to paused
    m_composingTimer->setSingleShot(true);
    m_composingTimer->setInterval(TYPING_TIMEOUT);
    m_composingTimer->callOnTimeout(this, [this] {
        sendChatState(QXmppMessage::Paused);

        // 10 seconds after user stopped typing, remove "paused" state
        m_stateTimeoutTimer->start(PAUSED_TYPING_TIMEOUT);
    });

    // Timer to reset typing-related notifications like paused and composing to active
    m_stateTimeoutTimer->setSingleShot(true);
    m_stateTimeoutTimer->callOnTimeout(this, [this] {
        sendChatState(QXmppMessage::Active);
    });

    // Timer to time out active state
    m_inactiveTimer->setSingleShot(true);
    m_inactiveTimer->setInterval(ACTIVE_TIMEOUT);
    m_inactiveTimer->callOnTimeout(this, [this] {
        sendChatState(QXmppMessage::Inactive);
    });

    // Timer to reset the chat partners state
    // if they lost connection while a state other then gone was active
    m_chatPartnerChatStateTimeout->setSingleShot(true);
    m_chatPartnerChatStateTimeout->setInterval(ACTIVE_TIMEOUT);
    m_chatPartnerChatStateTimeout->callOnTimeout(this, [this] {
        m_chatPartnerChatState = QXmppMessage::Gone;
        m_chatStateCache.insert(m_chatJid, QXmppMessage::Gone);
        Q_EMIT chatStateChanged();
    });
}

ChatController::~ChatController()
{
    s_instance = nullptr;
}

QString ChatController::accountJid()
{
    return m_accountJid;
}

QString ChatController::chatJid()
{
    return m_chatJid;
}

void ChatController::setChat(const QString &accountJid, const QString &chatJid)
{
    resetChat(accountJid, chatJid);

    // Send "active" state for the current chat.
    sendChatState(QXmppMessage::State::Active);

    // The encryption for group chats is initialized once all group chat user JIDs are fetched by
    // the GroupChatController.
    if (accountJid == chatJid) {
        EncryptionController::instance()->initializeAccount(accountJid);
    } else if (!rosterItem().isGroupChat()) {
        EncryptionController::instance()->initializeChat(accountJid, {chatJid});
    }
}

void ChatController::resetChat()
{
    resetChat({}, {});
}

bool ChatController::isChatCurrentChat(const QString &accountJid, const QString &chatJid) const
{
    return accountJid == m_accountJid && chatJid == m_chatJid;
}

const RosterItem &ChatController::rosterItem() const
{
    return m_rosterItemWatcher.item();
}

EncryptionWatcher *ChatController::accountEncryptionWatcher() const
{
    return m_accountEncryptionWatcher;
}

EncryptionWatcher *ChatController::chatEncryptionWatcher() const
{
    return m_chatEncryptionWatcher;
}

Encryption::Enum ChatController::activeEncryption() const
{
    return isEncryptionEnabled() ? encryption() : Encryption::NoEncryption;
}

bool ChatController::isEncryptionEnabled() const
{
    return encryption() != Encryption::NoEncryption && hasUsableEncryptionDevices();
}

Encryption::Enum ChatController::encryption() const
{
    return rosterItem().encryption;
}

void ChatController::setEncryption(Encryption::Enum encryption)
{
    RosterModel::instance()->setItemEncryption(m_accountJid, m_chatJid, encryption);
    Q_EMIT encryptionChanged();
}

void ChatController::resetComposingChatState()
{
    m_composingTimer->stop();
    m_stateTimeoutTimer->stop();

    // Reset composing chat state after message is sent
    sendChatState(QXmppMessage::State::Active);
}

QXmppMessage::State ChatController::chatState() const
{
    return m_chatPartnerChatState;
}

void ChatController::sendChatState(QXmppMessage::State state)
{
    if (m_contactResourcesWatcher.resourcesCount() == 0 || !rosterItem().chatStateSendingEnabled) {
        return;
    }

    // Handle some special cases
    switch (QXmppMessage::State(state)) {
    case QXmppMessage::State::Composing:
        // Restart timer if new character was typed in
        m_composingTimer->start();
        break;
    case QXmppMessage::State::Active:
        // Start inactive timer when active was sent,
        // so we can set the state to inactive two minutes later
        m_inactiveTimer->start();
        m_composingTimer->stop();
        break;
    default:
        break;
    }

    // Only send if the state changed, filter duplicated
    if (state != m_ownChatState) {
        m_ownChatState = state;
        MessageController::instance()->sendChatState(m_chatJid, rosterItem().isGroupChat(), state);
    }
}

void ChatController::sendChatState(ChatState::State state)
{
    sendChatState(QXmppMessage::State(state));
}

void ChatController::handleChatState(const QString &bareJid, QXmppMessage::State state)
{
    m_chatStateCache[bareJid] = state;

    if (bareJid == m_chatJid) {
        m_chatPartnerChatState = state;
        m_chatPartnerChatStateTimeout->start();
        Q_EMIT chatStateChanged();
    }
}

bool ChatController::hasUsableEncryptionDevices() const
{
    if (rosterItem().isGroupChat()) {
        return m_chatEncryptionWatcher->hasUsableDevices();
    }

    if (m_accountJid == m_chatJid) {
        return m_accountEncryptionWatcher->hasUsableDevices();
    }

    return m_chatEncryptionWatcher->hasUsableDevices();
}

void ChatController::resetChat(const QString &accountJid, const QString &chatJid)
{
    // Send "gone" state for the previous chat.
    if (!m_accountJid.isEmpty() && !m_chatJid.isEmpty()) {
        sendChatState(QXmppMessage::State::Gone);
    }

    m_rosterItemWatcher.setJid(chatJid);
    m_contactResourcesWatcher.setJid(chatJid);

    m_accountEncryptionWatcher->setAccountJid(accountJid);
    m_accountEncryptionWatcher->setJids({accountJid});

    m_chatEncryptionWatcher->setAccountJid(accountJid);

    // The JIDs for group chats are set by the GroupChatController.
    if (!rosterItem().isGroupChat()) {
        m_chatEncryptionWatcher->setJids({chatJid});
    }

    // Setting the following attributes must be done before sending chat states for the new chat.
    // Otherwise, the chat states would be sent for the previous chat.
    // Emitting "chatChanged()" must be done after "m_rosterItemWatcher.setJid(chatJid)".
    // Otherwise, "m_rosterItemWatcher" would not be initialized for the new chat.
    m_accountJid = accountJid;
    m_chatJid = chatJid;
    Q_EMIT accountJidChanged(accountJid);
    Q_EMIT chatJidChanged(chatJid);
    Q_EMIT chatChanged();

    // Reset the chat states of the previous chat.
    m_ownChatState = QXmppMessage::State::None;
    m_chatPartnerChatState = m_chatStateCache.value(chatJid, QXmppMessage::State::Gone);
    m_composingTimer->stop();
    m_stateTimeoutTimer->stop();
    m_inactiveTimer->stop();
    m_chatPartnerChatStateTimeout->stop();

    MessageModel::instance()->removeAllMessages();
}

#include "moc_ChatController.cpp"
