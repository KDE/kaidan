// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// QXmpp
#include <QXmppMessageReceiptManager.h>
// Kaidan
#include "Message.h"

class AccountSettings;
class ClientController;
class Connection;
class EncryptionController;
class FileSharingController;
class QXmppClient;
class QXmppMamManager;
class QXmppMessage;
class QXmppMessageReceiptManager;
class RosterController;

class MessageController : public QObject
{
    Q_OBJECT

public:
    MessageController(AccountSettings *accountSettings,
                      Connection *connection,
                      EncryptionController *encryptionController,
                      RosterController *rosterController,
                      FileSharingController *fileSharingController,
                      QXmppClient *client,
                      QXmppMamManager *mamManager,
                      QXmppMessageReceiptManager *messageReceiptManager,
                      QObject *parent = nullptr);

    QFuture<QXmpp::SendResult> send(QXmppMessage &&message, Encryption::Enum encryption, const QList<QString> &encryptionGroupChatUserJids = {});

    void sendMessageWithUndecidedEncryption(Message message);

    void sendPendingData();

    void sendChatState(const QString &chatJid,
                       bool isGroupChat,
                       const QXmppMessage::State state,
                       Encryption::Enum encryption,
                       const QList<QString> &encryptionJids = {});
    Q_SIGNAL void chatStateReceived(const QString &senderJid, QXmppMessage::State state);

    /**
     * Sends a chat marker for a read message.
     *
     * @param chatJid bare JID of the chat that contains the read message
     * @param messageId ID of the read message
     */
    void sendReadMarker(const QString &chatJid,
                        const QString &messageId,
                        Encryption::Enum encryption = Encryption::NoEncryption,
                        const QList<QString> &encryptionJids = {});

    Q_SIGNAL void contactMessageRead(const QString &accountJid, const QString &chatJid);

    QFuture<QXmpp::SendResult> sendMessageReaction(const QString &chatJid,
                                                   const QString &messageId,
                                                   bool isGroupChatMessage,
                                                   const QList<QString> &emojis,
                                                   Encryption::Enum encryption,
                                                   const QList<QString> &encryptionJids);

    void updateMessageReactionsAfterSending(const QString &chatJid, const QString &messageId, const QString &senderJid);

    void sendPendingMessage(Message message);
    void sendPendingMessageWithUploadedFiles(Message message);

    QFuture<bool> retrieveBacklogMessages(const QString &jid, bool isGroupChat, const QString &oldestMessageStanzaId = QLatin1String(""));

private:
    void handleRosterReceived(const QString &accountJid);
    void retrieveInitialMessages();

    /**
     * Retrieves one message before offsetMessageId with a body or shared files.
     *
     * offsetMessageId must be "" instead of a default-consctructed string to retrieve the latest
     * message with the given JID
     */
    void retrieveInitialMessage(const QString &jid, bool isGroupChat, const QString &offsetMessageId = QLatin1String(""));
    void retrieveAllMessages(const QString &groupChatJid = {});
    void retrieveCatchUpMessages(const QString &latestMessageStanzaId, const QString &groupChatJid = {});

    /**
     * Handles incoming messages from the server.
     */
    void handleMessage(const QXmppMessage &msg, MessageOrigin origin);

    void updateLatestMessage(const QString &chatJid, const QString &stanzaId, const QDateTime &timestamp, bool receivedFromGroupChat);

    /**
     * Handles a message that may contain a read marker.
     *
     * @return whether the message is handled because it contains a read marker
     */
    bool handleReadMarker(const QXmppMessage &message, const QString &senderJid, const QString &recipientJid, bool isOwnMessage);

    bool handleReaction(const QXmppMessage &message, const QString &chatJid, const QString &senderJid);
    bool handleFileSourcesAttachments(const QXmppMessage &message, const QString &chatJid);

    bool handleCorrection(const Message &message, const QString &accountJid, const QString &chatJid, const QString &replaceId, MessageOrigin origin);

    /**
     * Updates a message's delivery state and stores the received "stanzaId" if the message is sent
     * from this device and reflected from a group chat or the chat with oneself.
     *
     * The "stanzaId" is updated to be used when the message is referenced (e.g., if message reactions
     * are sent for it).
     */
    static bool updateReflectedMessage(Message &message, const QString &stanzaId);

    static std::optional<EncryptedSource> parseEncryptedSource(qint64 fileId, const QXmppEncryptedFileSource &source);
    static void parseSharedFiles(const QXmppMessage &message, Message &messageToEdit);
    static std::optional<File> parseOobUrl(const QXmppOutOfBandUrl &url, qint64 fileGroupId);

    void sendPendingMessages();
    void sendPendingMessageReactions();
    void sendPendingReadMarkers();

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    EncryptionController *const m_encryptionController;
    RosterController *const m_rosterController;
    FileSharingController *const m_fileSharingController;
    QXmppClient *const m_client;
    QXmppMamManager *const m_mamManager;
    QXmppMessageReceiptManager *const m_messageReceiptManager;
};
