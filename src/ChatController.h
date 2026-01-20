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

// Kaidan
#include "RosterItemWatcher.h"

class Account;
class ChatHintModel;
class ChatStateController;
class EncryptionController;
class EncryptionWatcher;
class GroupChatController;
class MessageController;
class MessageModel;
class NotificationController;

class ChatController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(Account *account READ account NOTIFY accountChanged)
    Q_PROPERTY(QString jid READ jid NOTIFY jidChanged)

    Q_PROPERTY(const RosterItem &rosterItem READ rosterItem NOTIFY rosterItemChanged)

    Q_PROPERTY(ChatStateController *chatStateController READ chatStateController NOTIFY chatStateControllerChanged)

    Q_PROPERTY(EncryptionWatcher *accountEncryptionWatcher READ accountEncryptionWatcher CONSTANT)
    Q_PROPERTY(EncryptionWatcher *chatEncryptionWatcher READ chatEncryptionWatcher CONSTANT)

    Q_PROPERTY(ChatHintModel *chatHintModel READ chatHintModel NOTIFY chatHintModelChanged)
    Q_PROPERTY(MessageModel *messageModel READ messageModel NOTIFY messageModelChanged)

    Q_PROPERTY(bool isEncryptionEnabled READ isEncryptionEnabled NOTIFY isEncryptionEnabledChanged)
    Q_PROPERTY(Encryption::Enum encryption READ encryption WRITE setEncryption NOTIFY encryptionChanged)
    Q_PROPERTY(QList<QString> groupChatUserJids READ groupChatUserJids NOTIFY groupChatUserJidsChanged)
    Q_PROPERTY(QString chatStateText READ chatStateText NOTIFY chatStateTextChanged)
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

    ChatStateController *chatStateController() const;
    Q_SIGNAL void chatStateControllerChanged();

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

    QString chatStateText() const;
    Q_SIGNAL void chatStateTextChanged();

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
    void handleChatStateChanged(const QString &jid);

    void executeOnceConnected(std::function<void()> &&function);

    void addConnection(const QMetaObject::Connection &connection);
    void removeConnections();

    void resetPreviousChat();

    Account *m_account = nullptr;
    QString m_jid;

    ChatStateController *m_chatStateController = nullptr;
    EncryptionController *m_encryptionController = nullptr;
    GroupChatController *m_groupChatController = nullptr;
    MessageController *m_messageController = nullptr;
    NotificationController *m_notificationController = nullptr;

    RosterItemWatcher m_rosterItemWatcher;

    EncryptionWatcher *const m_accountEncryptionWatcher;
    EncryptionWatcher *const m_chatEncryptionWatcher;

    ChatHintModel *m_chatHintModel = nullptr;
    MessageModel *m_messageModel = nullptr;

    QList<QString> m_groupChatUserJids;
    QString m_messageBodyToForward;

    QList<QMetaObject::Connection> m_connections;
};
