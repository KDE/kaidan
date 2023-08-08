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

class ClientWorker;
class QXmppMamManager;
class QXmppMessage;
class QXmppResultSetReply;

/**
 * @class MessageHandler Handler for incoming and outgoing messages.
 */
class MessageHandler : public QObject
{
	Q_OBJECT

public:
	MessageHandler(ClientWorker *clientWorker, QXmppClient *client, QObject *parent = nullptr);

	QFuture<QXmpp::SendResult> send(QXmppMessage &&message);

	void handleRosterReceived();
	void handleLastMessageStampFetched(const QDateTime &stamp);

	/**
	 * Handles incoming messages from the server.
	 */
	void handleMessage(const QXmppMessage &msg, MessageOrigin origin);

	/**
	 * Send a text message to any JID
	 */
	void sendMessage(const QString &toJid, const QString &body, bool isSpoiler, const QString &spoilerHint);

	/**
	 * Sends a chat state notification to the server.
	 */
	void sendChatState(const QString &toJid, const QXmppMessage::State state);

	/**
	 * Sends the corrected version of a message.
	 */
	void sendCorrectedMessage(Message msg);

	/**
	 * Sends a chat marker for a read message.
	 *
	 * @param chatJid bare JID of the chat that contains the read message
	 * @param messageId ID of the read message
	 */
	void sendReadMarker(const QString &chatJid, const QString &messageId);

	QFuture<QXmpp::SendResult> sendMessageReaction(const QString &chatJid, const QString &messageId, const QVector<QString> &emojis);

	void sendPendingMessage(Message message);

signals:
	void sendMessageRequested(const QString &toJid,
				  const QString &body,
				  bool isSpoiler,
				  const QString &spoilerHint);

	void retrieveBacklogMessagesRequested(const QString &jid, const QDateTime &stamp);

private:
	void handleConnected();

	/**
	 * Handles pending messages found in the database.
	 */
	void handlePendingMessages(const QVector<Message> &messages);

	void retrieveInitialMessages();
	void retrieveCatchUpMessages(const QDateTime &stamp);
	void retrieveBacklogMessages(const QString &jid, const QDateTime &last);

	/**
	 * Handles a message that may contain a read marker.
	 *
	 * @return whether the message is handled because it contains a read marker
	 */
	bool handleReadMarker(const QXmppMessage &message, const QString &senderJid, const QString &recipientJid, bool isOwnMessage);

	bool handleReaction(const QXmppMessage &message, const QString &senderJid);

	static void parseSharedFiles(const QXmppMessage &message, Message &messageToEdit);
	static std::optional<File> parseOobUrl(const QXmppOutOfBandUrl &url, qint64 fileGroupId);

	ClientWorker *m_clientWorker;
	QXmppClient *m_client;
	QXmppMessageReceiptManager m_receiptManager;
	QXmppMamManager *m_mamManager;

	QDateTime m_lastMessageStamp;
	bool m_lastMessageLoaded = false;

	uint m_runningInitialMessageQueries = 0;
};
