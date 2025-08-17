// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>

class Account;
class AccountSettings;
class ChatController;
class Connection;
class GroupChatController;
class NotificationController;
class QXmppPresence;
class RosterController;

class ChatHintButton
{
    Q_GADGET

    Q_PROPERTY(ChatHintButton::Type type MEMBER type)
    Q_PROPERTY(QString text MEMBER text)

public:
    enum Type {
        Dismiss,
        EnableAccount,
        ShowConnectionError,
        AllowPresenceSubscription,
        InviteContacts,
        Leave,
    };
    Q_ENUM(Type)

    Type type;
    QString text;

    bool operator==(const ChatHintButton &other) const = default;
};

Q_DECLARE_METATYPE(ChatHintButton)
Q_DECLARE_METATYPE(ChatHintButton::Type)

class ChatHintModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum class Role {
        Text = Qt::UserRole,
        Buttons,
        Loading,
        LoadingDescription,
    };

    ChatHintModel(Account *account, ChatController *chatController, QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QHash<int, QByteArray> roleNames() const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Q_INVOKABLE void handleButtonClicked(int i, ChatHintButton::Type type);

private:
    struct ChatHint {
        QString text;
        QList<ChatHintButton> buttons;
        bool loading = false;
        QString loadingDescription;

        bool operator==(const ChatHint &other) const = default;
    };

    void handleAccountEnabledChanged();

    void handleConnectionStateChanged();
    void handleConnectionErrorChanged();

    void handleUnrespondedPresenceSubscriptionRequests();
    void handlePresenceSubscriptionRequestReceived(const QXmppPresence &request);

    void handleNoGroupChatUsers();
    void checkGroupChatDeleted();
    void handleGroupChatDeleted(const QString &groupChatJid);

    int addEnableAccountChatHint();
    void addShowConnectionErrorChatHint();
    void addAllowPresenceSubscriptionChatHint(const QXmppPresence &request);
    int addInviteContactsChatHint();
    void addLeaveChatHint();

    void setLoading(int i, bool loading);

    int addChatHint(const ChatHint &chatHint);
    void insertChatHint(int i, const ChatHint &chatHint);
    void updateChatHint(ChatHintButton::Type buttonType, const std::function<void(ChatHint &)> &updateChatHint);
    void updateChatHint(int i, const std::function<void(ChatHint &)> &updateChatHint);
    void removeChatHint(ChatHintButton::Type buttonType);
    void removeChatHint(int i);

    int chatHintIndex(ChatHintButton::Type buttonType) const;
    bool hasButton(int i, ChatHintButton::Type buttonType) const;

    Account *const m_account;

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;

    GroupChatController *const m_groupChatController;
    NotificationController *const m_notificationController;
    RosterController *const m_rosterController;

    ChatController *const m_chatController;

    QList<ChatHint> m_chatHints;
};
