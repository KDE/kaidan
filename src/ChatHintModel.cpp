// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatHintModel.h"

// Qt
#include <QGuiApplication>
// Kaidan
#include "AccountController.h"
#include "ChatController.h"
#include "FutureUtils.h"
#include "GroupChatController.h"
#include "Kaidan.h"
#include "KaidanCoreLog.h"
#include "Notifications.h"
#include "QmlUtils.h"
#include "RosterController.h"
#include "RosterModel.h"

ChatHintModel *ChatHintModel::s_instance = nullptr;

ChatHintModel *ChatHintModel::instance()
{
    return s_instance;
}

ChatHintModel::ChatHintModel(QObject *parent)
    : QAbstractListModel(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    connect(Kaidan::instance(), &Kaidan::connectionStateChanged, this, &ChatHintModel::handleConnectionStateChanged);
    connect(Kaidan::instance(), &Kaidan::connectionErrorChanged, this, qOverload<>(&ChatHintModel::handleConnectionErrorChanged));

    connect(Kaidan::instance()->rosterController(),
            &RosterController::presenceSubscriptionRequestReceived,
            this,
            &ChatHintModel::handlePresenceSubscriptionRequestReceived);
    connect(GroupChatController::instance(), &GroupChatController::currentUserJidsChanged, this, &ChatHintModel::handleNoGroupChatUsers);
    connect(GroupChatController::instance(), &GroupChatController::groupChatDeleted, this, &ChatHintModel::handleGroupChatDeleted);
    connect(ChatController::instance(), &ChatController::chatChanged, this, [this]() {
        handleConnectionStateChanged();
        handleUnrespondedPresenceSubscriptionRequests();
        handleNoGroupChatUsers();
        checkGroupChatDeleted();
    });
}

ChatHintModel::~ChatHintModel() = default;

int ChatHintModel::rowCount(const QModelIndex &) const
{
    return m_chatHints.size();
}

QHash<int, QByteArray> ChatHintModel::roleNames() const
{
    return {
        {static_cast<int>(Role::Text), QByteArrayLiteral("text")},
        {static_cast<int>(Role::Buttons), QByteArrayLiteral("buttons")},
        {static_cast<int>(Role::Loading), QByteArrayLiteral("loading")},
        {static_cast<int>(Role::LoadingDescription), QByteArrayLiteral("loadingDescription")},
    };
}

QVariant ChatHintModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not get data from ChatHintModel:" << index << role;
        return {};
    }

    const auto &chatHint = m_chatHints.at(index.row());
    switch (static_cast<Role>(role)) {
    case Role::Text:
        return chatHint.text;
    case Role::Buttons:
        return QVariant::fromValue(chatHint.buttons);
    case Role::Loading:
        return chatHint.loading;
    case Role::LoadingDescription:
        return chatHint.loadingDescription;
    }

    qCWarning(KAIDAN_CORE_LOG, "Invalid role requested!");
    return {};
}

void ChatHintModel::handleButtonClicked(int i, ChatHintButton::Type type)
{
    switch (type) {
    case ChatHintButton::Dismiss:
        if (i == chatHintIndex(ChatHintButton::AllowPresenceSubscription)) {
            const auto jid = ChatController::instance()->chatJid();

            Kaidan::instance()->rosterController()->refuseSubscriptionToPresence(jid).then(this, [this, i, jid](bool succeeded) {
                if (succeeded) {
                    removeChatHint(i);
                } else {
                    setLoading(i, false);
                }
            });
        } else {
            removeChatHint(i);
        }

        return;
    case ChatHintButton::ConnectToServer:
        setLoading(i, true);
        Kaidan::instance()->logIn();
        return;
    case ChatHintButton::AllowPresenceSubscription: {
        setLoading(i, true);
        const auto jid = ChatController::instance()->chatJid();

        Kaidan::instance()->rosterController()->acceptSubscriptionToPresence(jid).then(this, [this, i, jid](bool succeeded) {
            if (succeeded) {
                removeChatHint(i);
            } else {
                setLoading(i, false);
            }
        });

        return;
    }
    case ChatHintButton::InviteContacts:
        setLoading(i, true);
        Q_EMIT GroupChatController::instance()->groupChatInviteeSelectionNeeded();
        removeChatHint(i);
        return;
    case ChatHintButton::Leave:
        setLoading(i, true);
        Q_EMIT GroupChatController::instance()->leaveGroupChat(ChatController::instance()->accountJid(), ChatController::instance()->chatJid());
        removeChatHint(i);
        return;
    default:
        return;
    }
}

void ChatHintModel::handleConnectionStateChanged()
{
    switch (Enums::ConnectionState(Kaidan::instance()->connectionState())) {
    case Enums::ConnectionState::StateDisconnected:
        if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer); i == -1) {
            handleConnectionErrorChanged(addConnectToServerChatHint(false));
        } else {
            updateChatHint(i, [](ChatHint &chatHint) {
                chatHint.loading = false;
            });

            handleConnectionErrorChanged(i);
        }

        return;
    case Enums::ConnectionState::StateConnecting:
        if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer); i == -1) {
            addConnectToServerChatHint(true);
        } else {
            updateChatHint(i, [](ChatHint &chatHint) {
                chatHint.loading = true;
            });
        }

        return;
    case Enums::ConnectionState::StateConnected:
        removeChatHint(ChatHintButton::ConnectToServer);
        return;
    default:
        return;
    }
}

void ChatHintModel::handleConnectionErrorChanged()
{
    updateChatHint(ChatHintButton::ConnectToServer, [](ChatHint &chatHint) {
        if (const auto error = ClientWorker::ConnectionError(Kaidan::instance()->connectionError()); error != ClientWorker::NoError) {
            chatHint.text = QmlUtils::connectionErrorMessage(error);
        }
    });
}

void ChatHintModel::handleConnectionErrorChanged(int i)
{
    updateChatHint(i, [](ChatHint &chatHint) {
        if (const auto error = ClientWorker::ConnectionError(Kaidan::instance()->connectionError()); error != ClientWorker::NoError) {
            chatHint.text = QmlUtils::connectionErrorMessage(error);
        }
    });
}

void ChatHintModel::handleUnrespondedPresenceSubscriptionRequests()
{
    const auto rosterController = Kaidan::instance()->rosterController();
    const auto chatController = ChatController::instance();
    const auto chatJid = chatController->chatJid();

    const auto unrespondedPresenceSubscriptionRequests = rosterController->unrespondedPresenceSubscriptionRequests();
    const auto i = chatHintIndex(ChatHintButton::AllowPresenceSubscription);

    if (unrespondedPresenceSubscriptionRequests.contains(chatJid)) {
        if (i == -1) {
            const auto request = unrespondedPresenceSubscriptionRequests.value(chatJid);
            addAllowPresenceSubscriptionChatHint(request);
        }
    } else if (i != -1) {
        removeChatHint(i);
        Notifications::instance()->closePresenceSubscriptionRequestNotification(chatController->accountJid(), chatJid);
    }
}

void ChatHintModel::handlePresenceSubscriptionRequestReceived(const QString &accountJid, const QXmppPresence &request)
{
    const auto subscriberJid = request.from();
    const auto rosterItem = RosterModel::instance()->findItem(subscriberJid);

    const auto notificationRule = rosterItem->notificationRule;
    const bool userMuted = (notificationRule == RosterItem::NotificationRule::Account
                            && AccountController::instance()->account().contactNotificationRule == Account::ContactNotificationRule::Never)
        || notificationRule == RosterItem::NotificationRule::Never;

    const bool requestForCurrentChat = request.from() == ChatController::instance()->chatJid();
    const bool chatActive = requestForCurrentChat && QGuiApplication::applicationState() == Qt::ApplicationActive;

    if (requestForCurrentChat) {
        addAllowPresenceSubscriptionChatHint(request);
    }

    if (!userMuted && !chatActive) {
        Notifications::instance()->sendPresenceSubscriptionRequestNotification(accountJid, subscriberJid);
    }
}

void ChatHintModel::handleNoGroupChatUsers()
{
    auto chatController = ChatController::instance();

    const auto currentGroupChatUserJids = GroupChatController::instance()->currentUserJids();
    const auto groupChatHasOtherUsers = currentGroupChatUserJids.size() > 1;
    const auto chatHintShouldBeShown = chatController->rosterItem().isGroupChat() && !groupChatHasOtherUsers;

    if (const auto i = chatHintIndex(ChatHintButton::InviteContacts); i == -1) {
        if (chatHintShouldBeShown) {
            addInviteContactsChatHint();
        }
    } else if (!chatHintShouldBeShown) {
        removeChatHint(i);
    }
}

void ChatHintModel::checkGroupChatDeleted()
{
    if (ChatController::instance()->rosterItem().isDeletedGroupChat()) {
        addLeaveChatHint();
    } else {
        removeChatHint(ChatHintButton::Type::Leave);
    }
}

void ChatHintModel::handleGroupChatDeleted(const QString &accountJid, const QString &groupChatJid)
{
    auto chatController = ChatController::instance();

    if (chatController->accountJid() == accountJid && chatController->chatJid() == groupChatJid) {
        addLeaveChatHint();
    }
}

int ChatHintModel::addConnectToServerChatHint(bool loading)
{
    return addChatHint(ChatHint{
        tr("You are not connected - Messages are sent and received once connected"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::ConnectToServer, tr("Connect")}},
        loading,
        tr("Connecting…"),
    });
}

void ChatHintModel::addAllowPresenceSubscriptionChatHint(const QXmppPresence &request)
{
    const QString text = [&request]() {
        const auto jid = ChatController::instance()->chatJid();
        const auto displayName = ChatController::instance()->rosterItem().displayName();
        const auto appendedText = request.statusText().isEmpty() ? QString() : QStringLiteral(": %1").arg(request.statusText());
        const auto oldJid = request.oldJid();

        if (oldJid.isEmpty()) {
            return tr("%1 would like to receive your personal data such as availability, devices and other personal information%2")
                .arg(displayName, appendedText);
        }

        const auto oldDisplayName = RosterModel::instance()->findItem(oldJid)->displayName();
        return tr("Your contact %1 (%2) is %3 (%4) now and would like to receive your personal data such as availability, devices and other personal "
                  "information again%5")
            .arg(oldDisplayName, oldJid, displayName, jid, appendedText);
    }();

    const auto chatHint = ChatHint{
        text,
        {ChatHintButton{ChatHintButton::Dismiss, tr("Refuse")}, ChatHintButton{ChatHintButton::AllowPresenceSubscription, tr("Allow")}},
        false,
        tr("Allowing…"),
    };

    // Ensure that the chat hint for connecting to the server is always on top.
    if (const auto i = chatHintIndex(ChatHintButton::AllowPresenceSubscription); i == -1) {
        addChatHint(chatHint);
    } else {
        insertChatHint(i, chatHint);
    }
}

int ChatHintModel::addInviteContactsChatHint()
{
    const auto chatHint = ChatHint{
        tr("This group has no users"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::InviteContacts, tr("Invite contacts")}},
        false,
        tr("Opening invitation area"),
    };

    auto i = chatHintIndex(ChatHintButton::ConnectToServer);

    if (i == -1) {
        return addChatHint(chatHint);
    }

    // Ensure that the chat hint for connecting to the server is always on top.
    insertChatHint(i, chatHint);

    return i;
}

void ChatHintModel::addLeaveChatHint()
{
    const auto chatHint = ChatHint{
        tr("This group is deleted"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::Leave, tr("Leave")}},
        false,
        tr("Leaving group chat"),
    };

    // Ensure that the chat hint for connecting to the server is always on top.
    if (const auto i = chatHintIndex(ChatHintButton::ConnectToServer); i == -1) {
        addChatHint(chatHint);
    } else {
        insertChatHint(i, chatHint);
    }
}

void ChatHintModel::setLoading(int i, bool loading)
{
    updateChatHint(i, [loading](ChatHint &chatHint) {
        chatHint.loading = loading;
    });
}

int ChatHintModel::addChatHint(const ChatHint &chatHint)
{
    const int i = m_chatHints.size();
    insertChatHint(i, chatHint);
    return i;
}

void ChatHintModel::insertChatHint(int i, const ChatHint &chatHint)
{
    beginInsertRows(QModelIndex(), i, i);
    m_chatHints.insert(i, chatHint);
    endInsertRows();
}

void ChatHintModel::updateChatHint(ChatHintButton::Type buttonType, const std::function<void(ChatHint &)> &updateChatHint)
{
    if (const auto i = chatHintIndex(buttonType); i != -1) {
        this->updateChatHint(i, updateChatHint);
    }
}

void ChatHintModel::updateChatHint(int i, const std::function<void(ChatHint &)> &updateChatHint)
{
    auto chatHint = m_chatHints.at(i);
    updateChatHint(chatHint);

    // Skip further processing in case of no changes.
    if (m_chatHints.at(i) == chatHint) {
        return;
    }

    m_chatHints.replace(i, chatHint);
    const auto modelIndex = index(i);
    Q_EMIT dataChanged(modelIndex, modelIndex);
}

void ChatHintModel::removeChatHint(ChatHintButton::Type buttonType)
{
    if (const auto i = chatHintIndex(buttonType); i != -1) {
        removeChatHint(i);
    }
}

void ChatHintModel::removeChatHint(int i)
{
    beginRemoveRows(QModelIndex(), i, i);
    m_chatHints.remove(i);
    endRemoveRows();
}

int ChatHintModel::chatHintIndex(ChatHintButton::Type buttonType) const
{
    for (int i = 0; i < m_chatHints.size(); ++i) {
        if (hasButton(i, buttonType)) {
            return i;
        }
    }

    return -1;
}

bool ChatHintModel::hasButton(int i, ChatHintButton::Type buttonType) const
{
    const auto buttons = m_chatHints.at(i).buttons;

    return std::any_of(buttons.cbegin(), buttons.cend(), [buttonType](const ChatHintButton &button) {
        return button.type == buttonType;
    });
}

#include "moc_ChatHintModel.cpp"
