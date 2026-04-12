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

// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "ChatHintModel.h"
#include "ChatStateCache.h"
#include "ChatStateController.h"
#include "EncryptionController.h"
#include "EncryptionWatcher.h"
#include "GroupChatController.h"
#include "GroupChatUserDb.h"
#include "MessageController.h"
#include "MessageModel.h"
#include "NotificationController.h"
#include "RosterModel.h"

ChatController::ChatController(QObject *parent)
    : QObject(parent)
    , m_accountEncryptionWatcher(new EncryptionWatcher(this))
    , m_chatEncryptionWatcher(new EncryptionWatcher(this))
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
    m_notificationController->setChatController(nullptr);
    resetPreviousChat();
}

void ChatController::initialize(Account *account, const QString &jid)
{
    Q_ASSERT(account);
    Q_ASSERT(!jid.isEmpty());
    Q_ASSERT(m_account != account || m_jid != jid);

    Q_EMIT aboutToChangeChat();

    if (m_account) {
        resetPreviousChat();
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

    initializeEncryption();
    initializeGroupChat();

    m_messageController = account->messageController();

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

ChatStateController *ChatController::chatStateController() const
{
    return m_chatStateController;
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

QString ChatController::chatStateText() const
{
    if (!m_account) {
        return {};
    }

    switch (m_account->chatStateCache()->chatState(m_jid)) {
    case QXmppMessage::State::Composing:
        return tr("%1 is typing…").arg(rosterItem().displayName());
    case QXmppMessage::State::Paused:
        return tr("%1 paused typing").arg(rosterItem().displayName());
    case QXmppMessage::State::Active:
    case QXmppMessage::State::Inactive:
    case QXmppMessage::State::Gone:
    case QXmppMessage::State::None:
        break;
    };

    return {};
}

QString ChatController::messageBodyToForward() const
{
    return m_messageBodyToForward;
}

void ChatController::setMessageBodyToForward(const QString &messageBodyToForward)
{
    if (m_messageBodyToForward != messageBodyToForward) {
        m_messageBodyToForward = messageBodyToForward;
        Q_EMIT messageBodyToForwardChanged();
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
    m_chatStateController = new ChatStateController(m_account->connection(), m_account->presenceCache(), this, m_messageController, this);
    Q_EMIT chatStateControllerChanged();

    addConnection(connect(m_account->chatStateCache(), &ChatStateCache::chatStateChanged, this, &ChatController::handleChatStateChanged));
    addConnection(connect(m_account->chatStateCache(), &ChatStateCache::chatStatesChanged, this, &ChatController::chatStateTextChanged));
    Q_EMIT chatStateTextChanged();
}

void ChatController::handleChatStateChanged(const QString &jid)
{
    if (jid == m_jid) {
        Q_EMIT chatStateTextChanged();
    }
}

void ChatController::executeOnceConnected(std::function<void()> &&function)
{
    const auto *connection = m_account->connection();

    if (connection->state() == Enums::ConnectionState::StateConnected) {
        function();
    } else {
        addConnection(connect(connection, &Connection::connected, this, std::move(function), Qt::SingleShotConnection));
    }
}

void ChatController::addConnection(const QMetaObject::Connection &connection)
{
    m_connections.append(connection);
}

void ChatController::removeConnections()
{
    std::ranges::for_each(m_connections, [](const QMetaObject::Connection &connection) {
        QObject::disconnect(connection);
    });
    m_connections.clear();
}

void ChatController::resetPreviousChat()
{
    removeConnections();

    m_chatHintModel->deleteLater();

    m_messageModel->removeAllMessages();
    m_messageModel->deleteLater();

    m_chatStateController->resetPreviousChat();
    m_chatStateController->deleteLater();
}

#include "moc_ChatController.cpp"
