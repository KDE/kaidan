// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppMessage.h>
// Kaidan
#include "PresenceCache.h"
#include "RosterItemWatcher.h"

class Account;
class ChatHintModel;
class EncryptionController;
class EncryptionWatcher;
class GroupChatController;
class MessageController;
class MessageModel;
class NotificationController;
class QTimer;

class ChatState : public QObject
{
    Q_OBJECT

public:
    enum State {
        None = QXmppMessage::None,
        Active = QXmppMessage::Active,
        Inactive = QXmppMessage::Inactive,
        Gone = QXmppMessage::Gone,
        Composing = QXmppMessage::Composing,
        Paused = QXmppMessage::Paused
    };
    Q_ENUM(State)
};

Q_DECLARE_METATYPE(ChatState::State)

class ChatController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Account *account READ account NOTIFY accountChanged)
    Q_PROPERTY(QString jid READ jid NOTIFY jidChanged)

    Q_PROPERTY(const RosterItem &rosterItem READ rosterItem NOTIFY rosterItemChanged)

    Q_PROPERTY(EncryptionWatcher *accountEncryptionWatcher READ accountEncryptionWatcher CONSTANT)
    Q_PROPERTY(EncryptionWatcher *chatEncryptionWatcher READ chatEncryptionWatcher CONSTANT)

    Q_PROPERTY(ChatHintModel *chatHintModel READ chatHintModel NOTIFY chatHintModelChanged)
    Q_PROPERTY(MessageModel *messageModel READ messageModel NOTIFY messageModelChanged)

    Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY isEncryptionEnabledChanged)
    Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
    Q_PROPERTY(Encryption::Enum activeEncryption READ activeEncryption NOTIFY encryptionChanged)
    Q_PROPERTY(QList<QString> groupChatUserJids READ groupChatUserJids NOTIFY groupChatUserJidsChanged)
    Q_PROPERTY(QXmppMessage::State chatState READ chatState NOTIFY chatStateChanged)
    Q_PROPERTY(QString messageBodyToForward READ messageBodyToForward WRITE setMessageBodyToForward NOTIFY messageBodyToForwardChanged)

public:
    explicit ChatController(QObject *parent = nullptr);
    ~ChatController();

    Q_INVOKABLE void initialize(Account *account, const QString &jid);
    Q_SIGNAL void aboutToChangeChat();
    Q_SIGNAL void chatChanged();

    Account *account() const;
    Q_SIGNAL void accountChanged();

    QString jid();
    void setJid(const QString &jid);
    Q_SIGNAL void jidChanged();

    const RosterItem &rosterItem() const;
    Q_SIGNAL void rosterItemChanged();

    EncryptionWatcher *accountEncryptionWatcher() const;
    EncryptionWatcher *chatEncryptionWatcher() const;

    ChatHintModel *chatHintModel() const;
    Q_SIGNAL void chatHintModelChanged();

    MessageModel *messageModel() const;
    Q_SIGNAL void messageModelChanged();

    Encryption::Enum activeEncryption() const;

    bool isEncryptionEnabled() const;
    Q_SIGNAL void isEncryptionEnabledChanged();

    Encryption::Enum encryption() const;
    void setEncryption(Encryption::Enum encryption);
    Q_SIGNAL void encryptionChanged();

    QList<QString> groupChatUserJids() const;
    Q_SIGNAL void groupChatUserJidsChanged();

    Q_INVOKABLE void resetComposingChatState();

    QXmppMessage::State chatState() const;
    Q_SIGNAL void chatStateChanged();

    Q_INVOKABLE void sendChatState(ChatState::State state);
    void sendChatState(QXmppMessage::State state);

    QString messageBodyToForward() const;
    void setMessageBodyToForward(const QString &messageBodyToForward);
    Q_SIGNAL void messageBodyToForwardChanged();

private:
    void initializeEncryption();
    bool hasUsableEncryptionDevices() const;

    void initializeGroupChat();
    void updateGroupChatUserJids(const QString &accountJid, const QString &groupChatJid);
    void updateGroupChatEncryption();
    void setGroupChatUserJids(const QList<QString> &groupChatUserJids);

    void initializeChatStateHandling();
    void resetChatStates();
    void handleChatState(const QString &bareJid, QXmppMessage::State state);

    void executeOnceConnected(std::function<void()> &&function);

    Account *m_account = nullptr;
    QString m_jid;

    EncryptionController *m_encryptionController = nullptr;
    GroupChatController *m_groupChatController = nullptr;
    MessageController *m_messageController = nullptr;
    NotificationController *m_notificationController = nullptr;

    RosterItemWatcher m_rosterItemWatcher;
    UserResourcesWatcher m_contactResourcesWatcher;
    EncryptionWatcher *const m_accountEncryptionWatcher;
    EncryptionWatcher *const m_chatEncryptionWatcher;

    ChatHintModel *m_chatHintModel = nullptr;
    MessageModel *m_messageModel = nullptr;

    QXmppMessage::State m_chatPartnerChatState = QXmppMessage::State::None;
    QXmppMessage::State m_ownChatState = QXmppMessage::State::None;
    QTimer *const m_composingTimer;
    QTimer *const m_stateTimeoutTimer;
    QTimer *const m_inactiveTimer;
    QTimer *const m_chatPartnerChatStateTimer;
    QMap<QString, QXmppMessage::State> m_chatStateCache;

    QList<QString> m_groupChatUserJids;
    QString m_messageBodyToForward;

    QList<QMetaObject::Connection> m_offlineConnections;
};
