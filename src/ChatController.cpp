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

// std
#include <chrono>
// Qt
#include <QTimer>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "ChatHintModel.h"
#include "EncryptionController.h"
#include "EncryptionWatcher.h"
#include "GroupChatController.h"
#include "GroupChatUserDb.h"
#include "MessageController.h"
#include "MessageModel.h"
#include "NotificationController.h"
#include "RosterModel.h"

using namespace std::chrono_literals;

constexpr auto PAUSED_TYPING_TIMEOUT = 10s;
constexpr auto ACTIVE_TIMEOUT = 2min;
constexpr auto TYPING_TIMEOUT = 2s;

ChatController::ChatController(QObject *parent)
    : QObject(parent)
    , m_accountEncryptionWatcher(new EncryptionWatcher(this))
    , m_chatEncryptionWatcher(new EncryptionWatcher(this))
    , m_composingTimer(new QTimer(this))
    , m_stateTimeoutTimer(new QTimer(this))
    , m_inactiveTimer(new QTimer(this))
    , m_chatPartnerChatStateTimer(new QTimer(this))
{
    connect(&m_rosterItemWatcher, &RosterItemWatcher::itemChanged, this, &ChatController::rosterItemChanged);
    connect(&m_rosterItemWatcher, &RosterItemWatcher::itemChanged, this, &ChatController::isEncryptionEnabledChanged);

    connect(this, &ChatController::encryptionChanged, this, &ChatController::isEncryptionEnabledChanged);

    connect(m_accountEncryptionWatcher, &EncryptionWatcher::hasUsableDevicesChanged, this, &ChatController::isEncryptionEnabledChanged);
    connect(m_chatEncryptionWatcher, &EncryptionWatcher::hasUsableDevicesChanged, this, &ChatController::isEncryptionEnabledChanged);

    connect(GroupChatUserDb::instance(), &GroupChatUserDb::userJidsChanged, this, &ChatController::updateGroupChatUserJids);
}

ChatController::~ChatController()
{
    sendChatState(QXmppMessage::State::Gone);
    m_notificationController->setChatController(nullptr);
}

void ChatController::initialize(Account *account, const QString &jid)
{
    Q_ASSERT(account);
    Q_ASSERT(!jid.isEmpty());
    Q_ASSERT(m_account != account || m_jid != jid);

    Q_EMIT aboutToChangeChat();

    std::ranges::for_each(m_offlineConnections, [](const QMetaObject::Connection &connection) {
        QObject::disconnect(connection);
    });
    m_offlineConnections.clear();

    // Reset the previous chat.
    if (m_account || !m_jid.isEmpty()) {
        resetChatStates();
        m_messageModel->removeAllMessages();
    }

    if (m_account != account) {
        m_account = account;
        Q_EMIT accountChanged();
    }

    if (m_jid != jid) {
        m_jid = jid;
        Q_EMIT jidChanged();
    }

    m_rosterItemWatcher.setAccountJid(account->settings()->jid());
    m_rosterItemWatcher.setJid(jid);

    m_contactResourcesWatcher.setJid(jid);
    m_contactResourcesWatcher.setPresenceCache(account->presenceCache());

    initializeEncryption();
    initializeGroupChat();

    m_messageController = account->messageController();
    connect(m_messageController, &MessageController::chatStateReceived, this, &ChatController::handleChatState);

    m_notificationController = account->notificationController();
    m_notificationController->setChatController(this);

    m_chatHintModel = new ChatHintModel(account, this, this);
    Q_EMIT chatHintModelChanged();

    m_messageModel = new MessageModel(account->settings(),
                                      account->connection(),
                                      account->atmController(),
                                      this,
                                      m_encryptionController,
                                      m_messageController,
                                      m_notificationController,
                                      this);
    Q_EMIT messageModelChanged();

    initializeChatStateHandling();
    sendChatState(QXmppMessage::State::Active);

    Q_EMIT chatChanged();
}

Account *ChatController::account() const
{
    return m_account;
}

QString ChatController::jid()
{
    return m_jid;
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

ChatHintModel *ChatController::chatHintModel() const
{
    return m_chatHintModel;
}

MessageModel *ChatController::messageModel() const
{
    return m_messageModel;
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
    RosterModel::instance()->setItemEncryption(m_account->settings()->jid(), m_jid, encryption);
    Q_EMIT encryptionChanged();
}

QList<QString> ChatController::groupChatUserJids() const
{
    return m_groupChatUserJids;
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

void ChatController::sendChatState(ChatState::State state)
{
    sendChatState(QXmppMessage::State(state));
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
        const auto encryption = activeEncryption();

        if (const auto isGroupChat = rosterItem().isGroupChat()) {
            m_messageController->sendChatState(m_jid, isGroupChat, state, encryption, m_groupChatUserJids);
        } else {
            m_messageController->sendChatState(m_jid, isGroupChat, state, encryption);
        }
    }
}

void ChatController::initializeEncryption()
{
    m_encryptionController = m_account->encryptionController();

    m_accountEncryptionWatcher->setEncryptionController(m_encryptionController);
    m_accountEncryptionWatcher->setAccountJid(m_account->settings()->jid());
    m_accountEncryptionWatcher->setJids({m_account->settings()->jid()});

    m_chatEncryptionWatcher->setEncryptionController(m_encryptionController);
    m_chatEncryptionWatcher->setAccountJid(m_account->settings()->jid());

    // The JIDs for group chats are set by initializeGroupChat().
    if (!rosterItem().isGroupChat()) {
        m_chatEncryptionWatcher->setJids({m_jid});
    }

    // The encryption for group chats is initialized by initializeGroupChat().
    if (m_account->settings()->jid() == m_jid) {
        executeOnceConnected([this]() {
            m_encryptionController->initializeAccount();
        });
    } else if (!rosterItem().isGroupChat()) {
        executeOnceConnected([this]() {
            m_encryptionController->initializeChat({m_jid});
        });
    }
}

bool ChatController::hasUsableEncryptionDevices() const
{
    if (!m_account) {
        return false;
    }

    if (rosterItem().isGroupChat()) {
        return m_chatEncryptionWatcher->hasUsableDevices();
    }

    if (m_account->settings()->jid() == m_jid) {
        return m_accountEncryptionWatcher->hasUsableDevices();
    }

    return m_chatEncryptionWatcher->hasUsableDevices();
}

void ChatController::initializeGroupChat()
{
    if (rosterItem().isGroupChat()) {
        m_groupChatController = m_account->groupChatController();

        const auto relevantAccountJid = m_account->settings()->jid();
        const auto relevantChatJid = m_jid;

        GroupChatUserDb::instance()
            ->userJids(relevantAccountJid, relevantChatJid)
            .then(this, [this, relevantAccountJid, relevantChatJid](QList<QString> &&userJids) {
                // Ensure that the chat is still the open one after fetching the user JIDs.
                if (m_account->settings()->jid() == relevantAccountJid && m_jid == relevantChatJid) {
                    setGroupChatUserJids(userJids);

                    // Handle the case when the database does not contain users for the current group chat.
                    // That happens, for example, after joining a channel or when the database was manually
                    // deleted.
                    //
                    // The encryption for group chats is initialized once all group chat user JIDs are
                    // fetched.
                    if (m_groupChatUserJids.contains(m_account->settings()->jid())) {
                        updateGroupChatEncryption();
                    } else {
                        m_groupChatController->requestGroupChatUsers(jid());
                    }
                }
            });
    } else {
        m_groupChatController = nullptr;
        setGroupChatUserJids({});
    }
}

void ChatController::updateGroupChatUserJids(const QString &accountJid, const QString &groupChatJid)
{
    if (accountJid == m_account->settings()->jid() && groupChatJid == m_jid) {
        GroupChatUserDb::instance()->userJids(accountJid, groupChatJid).then(this, [this, accountJid, groupChatJid](QList<QString> &&userJids) {
            // Ensure that the chat is still the open one after fetching the user JIDs.
            if (m_account->settings()->jid() == accountJid && m_jid == groupChatJid) {
                setGroupChatUserJids(userJids);
                updateGroupChatEncryption();
            }
        });
    }
}

void ChatController::updateGroupChatEncryption()
{
    auto jids = m_groupChatUserJids;
    jids.removeOne(m_account->settings()->jid());

    if (!jids.isEmpty()) {
        m_chatEncryptionWatcher->setJids(jids);

        executeOnceConnected([this, jids]() {
            m_encryptionController->initializeChat(jids);
        });
    }
}

void ChatController::setGroupChatUserJids(const QList<QString> &groupChatUserJids)
{
    if (m_groupChatUserJids != groupChatUserJids) {
        m_groupChatUserJids = groupChatUserJids;
        Q_EMIT groupChatUserJidsChanged();
    }
}

void ChatController::initializeChatStateHandling()
{
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
    m_chatPartnerChatStateTimer->setSingleShot(true);
    m_chatPartnerChatStateTimer->setInterval(ACTIVE_TIMEOUT);
    m_chatPartnerChatStateTimer->callOnTimeout(this, [this] {
        m_chatPartnerChatState = QXmppMessage::Gone;
        m_chatStateCache.insert(m_jid, QXmppMessage::Gone);
        Q_EMIT chatStateChanged();
    });
}

void ChatController::resetChatStates()
{
    sendChatState(QXmppMessage::State::Gone);

    m_ownChatState = QXmppMessage::State::None;
    m_chatPartnerChatState = m_chatStateCache.value(m_jid, QXmppMessage::State::Gone);
    m_composingTimer->stop();
    m_stateTimeoutTimer->stop();
    m_inactiveTimer->stop();
    m_chatPartnerChatStateTimer->stop();
}

void ChatController::handleChatState(const QString &bareJid, QXmppMessage::State state)
{
    m_chatStateCache[bareJid] = state;

    if (bareJid == m_jid) {
        m_chatPartnerChatState = state;
        m_chatPartnerChatStateTimer->start();
        Q_EMIT chatStateChanged();
    }
}

void ChatController::executeOnceConnected(std::function<void()> &&function)
{
    const auto *connection = m_account->connection();

    if (connection->state() == Enums::ConnectionState::StateConnected) {
        function();
    } else {
        m_offlineConnections.append(connect(connection, &Connection::connected, this, std::move(function), Qt::SingleShotConnection));
    }
}

#include "moc_ChatController.cpp"
