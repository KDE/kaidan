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

class QXmppMessage;

class MessageController : public QObject
{
    Q_OBJECT

public:
    static MessageController *instance();

    explicit MessageController(QObject *parent = nullptr);
    ~MessageController();

    QFuture<QXmpp::SendResult> send(QXmppMessage &&message);

    /**
     * Sends pending messages again after searching them in the database.
     */
    void sendPendingMessages();

    void sendPendingMessageReactions(const QString &accountJid);
    void updateMessageReactionsAfterSending(const QString &messageId, const QString &senderJid);

    /**
     * Sends a chat state notification to the server.
     */
    void sendChatState(const QString &toJid, bool isGroupChat, const QXmppMessage::State state);

    /**
     * Sends a chat marker for a read message.
     *
     * @param chatJid bare JID of the chat that contains the read message
     * @param messageId ID of the read message
     */
    void sendReadMarker(const QString &chatJid, const QString &messageId);

    QFuture<QXmpp::SendResult> sendMessageReaction(const QString &chatJid, const QString &messageId, bool isGroupChatMessage, const QList<QString> &emojis);

    void sendPendingMessage(Message message);

    QFuture<bool> retrieveBacklogMessages(const QString &jid, bool isGroupChat, const QString &oldestMessageStanzaId = QLatin1String(""));

private:
    void handleRosterReceived();
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

    void
    updateLatestMessage(const QString &accountJid, const QString &chatJid, const QString &stanzaId, const QDateTime &timestamp, bool receivedFromGroupChat);

    /**
     * Handles a message that may contain a read marker.
     *
     * @return whether the message is handled because it contains a read marker
     */
    bool handleReadMarker(const QXmppMessage &message, const QString &senderJid, const QString &recipientJid, bool isOwnMessage);

    bool handleReaction(const QXmppMessage &message, const QString &senderJid);
    bool handleFileSourcesAttachments(const QXmppMessage &message, const QString &chatJid);

    static std::optional<EncryptedSource> parseEncryptedSource(qint64 fileId, const QXmppEncryptedFileSource &source);
    static void parseSharedFiles(const QXmppMessage &message, Message &messageToEdit);
    static std::optional<File> parseOobUrl(const QXmppOutOfBandUrl &url, qint64 fileGroupId);

    static MessageController *s_instance;
};
