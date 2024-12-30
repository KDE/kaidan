// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractItemModel>

class QXmppPresence;

class ChatHintButton
{
    Q_GADGET

    Q_PROPERTY(ChatHintButton::Type type MEMBER type)
    Q_PROPERTY(QString text MEMBER text)

public:
    enum Type {
        Dismiss,
        ConnectToServer,
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

    static ChatHintModel *instance();

    ChatHintModel(QObject *parent = nullptr);
    ~ChatHintModel();

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

    void handleConnectionStateChanged();
    void handleConnectionErrorChanged();
    void handleConnectionErrorChanged(int i);

    void handleUnrespondedPresenceSubscriptionRequests();
    void handlePresenceSubscriptionRequestReceived(const QString &accountJid, const QXmppPresence &request);

    void handleNoGroupChatUsers();
    void checkGroupChatDeleted();
    void handleGroupChatDeleted(const QString &accountJid, const QString &groupChatJid);

    int addConnectToServerChatHint(bool loading = false);
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

    QList<ChatHint> m_chatHints;

    static ChatHintModel *s_instance;
};
