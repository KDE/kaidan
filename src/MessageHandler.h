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

	/**
	 * Sends pending messages again after searching them in the database.
	 */
	void sendPendingMessages();

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

	Q_SIGNAL void retrieveBacklogMessagesRequested(const QString &jid, const QDateTime &stamp);

private:
	void handleConnected();
	void handleRosterReceived();
	void retrieveInitialMessages();

	/**
	 * Retrieves one message before offsetMessageId with a body or shared files.
	 *
	 * offsetMessageId must be "" instead of a default-consctructed string to retrieve the latest
	 * message with the given JID
	 */
	void retrieveInitialMessage(const QString &jid, const QString &offsetMessageId = QLatin1String(""));
	void retrieveCatchUpMessages(const QString &latestMessageStanzaId);
	void retrieveBacklogMessages(const QString &jid, const QDateTime &last);

	/**
	 * Handles incoming messages from the server.
	 */
	void handleMessage(const QXmppMessage &msg, MessageOrigin origin);

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

	uint m_runningInitialMessageQueries = 0;
};
