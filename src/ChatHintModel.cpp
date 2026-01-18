// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ChatHintModel.h"

// Qt
#include <QGuiApplication>
// Kaidan
#include "Account.h"
#include "ChatController.h"
#include "GroupChatController.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "NotificationController.h"
#include "RosterController.h"
#include "RosterModel.h"

ChatHintModel::ChatHintModel(Account *account, ChatController *chatController, QObject *parent)
    : QAbstractListModel(parent)
    , m_account(account)
    , m_accountSettings(account->settings())
    , m_connection(account->connection())
    , m_groupChatController(account->groupChatController())
    , m_notificationController(account->notificationController())
    , m_rosterController(account->rosterController())
    , m_chatController(chatController)
{
    connect(m_accountSettings, &AccountSettings::enabledChanged, this, &ChatHintModel::handleAccountEnabledChanged);
    connect(m_connection, &Connection::stateChanged, this, &ChatHintModel::handleConnectionStateChanged);
    connect(m_connection, &Connection::errorChanged, this, &ChatHintModel::handleConnectionErrorChanged);

    connect(m_rosterController, &RosterController::presenceSubscriptionRequestReceived, this, &ChatHintModel::handlePresenceSubscriptionRequestReceived);
    connect(m_chatController, &ChatController::groupChatUserJidsChanged, this, &ChatHintModel::handleNoGroupChatUsers);

    if (m_groupChatController) {
        connect(m_groupChatController, &GroupChatController::groupChatDeleted, this, &ChatHintModel::handleGroupChatDeleted);
    }

    connect(m_chatController, &ChatController::chatChanged, this, [this]() {
        handleAccountEnabledChanged();
        handleConnectionStateChanged();
        handleConnectionErrorChanged();
        handleUnrespondedPresenceSubscriptionRequests();
        handleNoGroupChatUsers();
        checkGroupChatDeleted();
    });
}

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
            if (m_rosterController->refuseSubscriptionToPresence(m_chatController->jid())) {
                removeChatHint(i);
            } else {
                setLoading(i, false);
            }
        } else {
            removeChatHint(i);
        }

        return;
    case ChatHintButton::EnableAccount:
        setLoading(i, true);
        m_account->enable();
        return;
    case ChatHintButton::ShowConnectionError:
        Q_EMIT MainController::instance()->openGlobalDrawerRequested();
        return;
    case ChatHintButton::AllowPresenceSubscription: {
        setLoading(i, true);

        if (m_rosterController->acceptSubscriptionToPresence(m_chatController->jid())) {
            removeChatHint(i);
        } else {
            setLoading(i, false);
        }

        return;
    }
    case ChatHintButton::InviteContacts:
        setLoading(i, true);
        Q_EMIT m_groupChatController->groupChatInviteeSelectionNeeded();
        removeChatHint(i);
        return;
    case ChatHintButton::Leave:
        setLoading(i, true);
        Q_EMIT m_groupChatController->leaveGroupChat(m_chatController->jid());
        removeChatHint(i);
        return;
    default:
        return;
    }
}

void ChatHintModel::handleAccountEnabledChanged()
{
    if (m_accountSettings->enabled()) {
        removeChatHint(ChatHintButton::EnableAccount);
    } else {
        addEnableAccountChatHint();
    }
}

void ChatHintModel::handleConnectionStateChanged()
{
    if (m_connection->state() == Enums::ConnectionState::StateConnecting) {
        if (const auto i = chatHintIndex(ChatHintButton::ShowConnectionError); i != -1) {
            setLoading(i, true);
        }
    } else {
        if (const auto i = chatHintIndex(ChatHintButton::ShowConnectionError); i != -1) {
            setLoading(i, false);
        }
    }
}

void ChatHintModel::handleConnectionErrorChanged()
{
    const auto chatHintShouldBeShown = m_connection->error() != ClientController::NoError;

    if (const auto i = chatHintIndex(ChatHintButton::ShowConnectionError); i == -1) {
        if (chatHintShouldBeShown) {
            addShowConnectionErrorChatHint();
        }
    } else if (!chatHintShouldBeShown) {
        removeChatHint(i);
    }
}

void ChatHintModel::handleUnrespondedPresenceSubscriptionRequests()
{
    const auto chatJid = m_chatController->jid();

    const auto unrespondedPresenceSubscriptionRequests = m_rosterController->unrespondedPresenceSubscriptionRequests();
    const auto i = chatHintIndex(ChatHintButton::AllowPresenceSubscription);

    if (unrespondedPresenceSubscriptionRequests.contains(chatJid)) {
        if (i == -1) {
            const auto request = unrespondedPresenceSubscriptionRequests.value(chatJid);
            addAllowPresenceSubscriptionChatHint(request);
        }
    } else if (i != -1) {
        removeChatHint(i);
        m_notificationController->closePresenceSubscriptionRequestNotification(chatJid);
    }
}

void ChatHintModel::handlePresenceSubscriptionRequestReceived(const QXmppPresence &request)
{
    const auto subscriberJid = request.from();
    const auto rosterItem = RosterModel::instance()->item(m_accountSettings->jid(), subscriberJid);

    const auto notificationRule = rosterItem->notificationRule;
    const bool userMuted = (notificationRule == RosterItem::NotificationRule::Account
                            && m_accountSettings->contactNotificationRule() == AccountSettings::ContactNotificationRule::Never)
        || notificationRule == RosterItem::NotificationRule::Never;

    const bool requestForCurrentChat = request.from() == m_chatController->jid();
    const bool chatActive = requestForCurrentChat && QGuiApplication::applicationState() == Qt::ApplicationActive;

    if (requestForCurrentChat) {
        addAllowPresenceSubscriptionChatHint(request);
    }

    if (!userMuted && !chatActive) {
        m_notificationController->sendPresenceSubscriptionRequestNotification(subscriberJid);
    }
}

void ChatHintModel::handleNoGroupChatUsers()
{
    auto chatController = m_chatController;

    const auto groupChatUserJids = m_chatController->groupChatUserJids();
    const auto groupChatHasOtherUsers = groupChatUserJids.size() > 1;
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
    if (m_chatController->rosterItem().isDeletedGroupChat()) {
        addLeaveChatHint();
    } else {
        removeChatHint(ChatHintButton::Type::Leave);
    }
}

void ChatHintModel::handleGroupChatDeleted(const QString &groupChatJid)
{
    auto chatController = m_chatController;

    if (chatController->jid() == groupChatJid) {
        addLeaveChatHint();
    }
}

int ChatHintModel::addEnableAccountChatHint()
{
    return addChatHint({
        tr("Your account is not enabled - Messages are sent and received once enabled"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::EnableAccount, tr("Enable")}},
        false,
        tr("Enabling…"),
    });
}

void ChatHintModel::addShowConnectionErrorChatHint()
{
    addChatHint({
        tr("Could not connect"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::ShowConnectionError, tr("Show error")}},
        false,
        tr("Connecting…"),
    });
}

void ChatHintModel::addAllowPresenceSubscriptionChatHint(const QXmppPresence &request)
{
    const QString text = [this, &request]() {
        const auto jid = m_chatController->jid();
        const auto displayName = m_chatController->rosterItem().displayName();
        const auto appendedText = request.statusText().isEmpty() ? QString() : QStringLiteral(": %1").arg(request.statusText());
        const auto oldJid = request.oldJid();

        if (oldJid.isEmpty()) {
            return tr("%1 would like to receive your personal data such as availability, devices and other personal information%2")
                .arg(displayName, appendedText);
        }

        const auto oldDisplayName = RosterModel::instance()->item(m_accountSettings->jid(), oldJid)->displayName();
        return tr("Your contact %1 (%2) is %3 (%4) now and would like to receive your personal data such as availability, devices and other personal "
                  "information again%5")
            .arg(oldDisplayName, oldJid, displayName, jid, appendedText);
    }();

    const ChatHint chatHint = {
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
    const ChatHint chatHint = {
        tr("This group has no users"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::InviteContacts, tr("Invite contacts")}},
        false,
        tr("Opening invitation area"),
    };

    auto i = chatHintIndex(ChatHintButton::EnableAccount);

    if (i == -1) {
        return addChatHint(chatHint);
    }

    // Ensure that the chat hint for connecting to the server is always on top.
    insertChatHint(i, chatHint);

    return i;
}

void ChatHintModel::addLeaveChatHint()
{
    const ChatHint chatHint = {
        tr("This group is deleted"),
        {ChatHintButton{ChatHintButton::Dismiss, tr("Dismiss")}, ChatHintButton{ChatHintButton::Leave, tr("Leave")}},
        false,
        tr("Leaving group chat"),
    };

    // Ensure that the chat hint for connecting to the server is always on top.
    if (const auto i = chatHintIndex(ChatHintButton::EnableAccount); i == -1) {
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
