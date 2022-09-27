/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2022 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "MessageHandler.h"
// Qt
#include <QUrl>
// QXmpp
#include <QXmppE2eeMetadata.h>
#include <QXmppMamManager.h>
#include <QXmppRosterManager.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "ClientWorker.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "Database.h"
#include "Message.h"
#include "MessageDb.h"
#include "MessageModel.h"
#include "MediaUtils.h"
#include "Notifications.h"
#include "RosterModel.h"

// Number of messages fetched at once when loading MAM backlog
constexpr int MAM_BACKLOG_FETCH_COUNT = 40;

QXmppMessage toQXmppMessage(const Message &msg)
{
	QXmppE2eeMetadata e2ee;
	e2ee.setSenderKey(msg.senderKey);

	QXmppMessage q;
	q.setId(msg.id);
	q.setTo(msg.to);
	q.setFrom(msg.from);
	q.setBody(msg.body);
	q.setStamp(msg.stamp);
	q.setIsSpoiler(msg.isSpoiler);
	q.setSpoilerHint(msg.spoilerHint);
	q.setMarkable(msg.isMarkable);
	q.setMarker(msg.marker);
	q.setMarkerId(msg.markerId);
	q.setOutOfBandUrl(msg.outOfBandUrl);
	q.setReplaceId(msg.replaceId);
	q.setOriginId(msg.originId);
	q.setStanzaId(msg.stanzaId);
	q.setReceiptRequested(msg.receiptRequested);
	q.setE2eeMetadata(e2ee);

	return q;
}

MessageHandler::MessageHandler(ClientWorker *clientWorker, QXmppClient *client, QObject *parent)
	: QObject(parent),
	  m_clientWorker(clientWorker),
	  m_client(client),
	  m_mamManager(client->findExtension<QXmppMamManager>())
{
	connect(client, &QXmppClient::messageReceived, this, [this](const QXmppMessage &msg) {
		handleMessage(msg, MessageOrigin::Stream);
	});
	connect(this, &MessageHandler::sendMessageRequested, this, &MessageHandler::sendMessage);
	connect(this, &MessageHandler::sendReadMarkerRequested, this, &MessageHandler::sendReadMarker);
	connect(MessageModel::instance(), &MessageModel::sendCorrectedMessageRequested,
	        this, &MessageHandler::sendCorrectedMessage);
	connect(MessageModel::instance(), &MessageModel::sendChatStateRequested,
	        this, &MessageHandler::sendChatState);

	connect(client, &QXmppClient::connected, this, &MessageHandler::handleConnected);
	connect(client, &QXmppClient::disconnected, this, &MessageHandler::handleDisonnected);
	connect(client->findExtension<QXmppRosterManager>(), &QXmppRosterManager::rosterReceived,
	        this, &MessageHandler::handleRosterReceived);
	connect(MessageDb::instance(), &MessageDb::lastMessageStampFetched,
	        this, &MessageHandler::handleLastMessageStampFetched);

	connect(&m_receiptManager, &QXmppMessageReceiptManager::messageDelivered,
		this, [this](const QString &, const QString &id) {
		MessageDb::instance()->updateMessage(id, [](Message &msg) {
			msg.deliveryState = Enums::DeliveryState::Delivered;
			msg.errorText.clear();
		});
	});

	connect(m_mamManager, &QXmppMamManager::archivedMessageReceived, this, &MessageHandler::handleArchiveMessage);
	connect(m_mamManager, &QXmppMamManager::resultsRecieved, this, &MessageHandler::handleArchiveResults);

	connect(this, &MessageHandler::retrieveBacklogMessagesRequested, this, &MessageHandler::retrieveBacklogMessages);

	client->addExtension(&m_receiptManager);

	connect(MessageModel::instance(), &MessageModel::pendingMessagesFetched,
			this, &MessageHandler::handlePendingMessages);

	// get last message stamp to retrieve all new messages from the server since then
	MessageDb::instance()->fetchLastMessageStamp();
}

MessageHandler::~MessageHandler()
{
	delete m_mamManager;
}

void MessageHandler::handleRosterReceived()
{
	// retrieve initial messages for each contact, if there is no last message locally
	if (m_lastMessageLoaded && m_lastMessageStamp.isNull())
		retrieveInitialMessages();
}

void MessageHandler::handleLastMessageStampFetched(const QDateTime &stamp)
{
	m_lastMessageStamp = stamp;
	m_lastMessageLoaded = true;

	// this is for the case that loading the last message took longer than connecting to
	// the server:

	// if already connected directly retrieve messages
	if (m_client->isConnected()) {
		// if there are no messages at all, load initial history,
		// otherwise load all missed messages since last online.
		if (stamp.isNull()) {
			// only start if roster was received already
			if (m_client->findExtension<QXmppRosterManager>()->isRosterReceived())
				retrieveInitialMessages();
		} else {
			retrieveCatchUpMessages(stamp);
		}
	}
}

void MessageHandler::handleMessage(const QXmppMessage &msg, MessageOrigin origin)
{
	if (msg.type() == QXmppMessage::Error) {
		MessageDb::instance()->updateMessage(msg.id(), [errorText { msg.error().text() }](Message &msg) {
			msg.deliveryState = Enums::DeliveryState::Error;
			msg.errorText = errorText;
		});
		return;
	}

	const auto senderJid = QXmppUtils::jidToBareJid(msg.from());
	const auto recipientJid = QXmppUtils::jidToBareJid(msg.to());
	const auto isOwnMessage = senderJid == m_client->configuration().jidBare();

	if (msg.state() != QXmppMessage::State::None) {
		emit MessageModel::instance()->handleChatStateRequested(
				senderJid, msg.state());
	}

	if (handleReadMarker(msg, senderJid, recipientJid, isOwnMessage)) {
		return;
	}

	if (msg.body().isEmpty() && msg.outOfBandUrl().isEmpty()) {
		return;
	}

	Message message;
	message.from = senderJid;
	message.to = recipientJid;
	message.isOwn = isOwnMessage;
	message.id = msg.id();

	if (auto e2eeMetadata = msg.e2eeMetadata()) {
		message.encryption = Encryption::Enum(e2eeMetadata->encryption());
		message.senderKey = e2eeMetadata->senderKey();
	}

	if (auto encryptionName = msg.encryptionName();
			!encryptionName.isEmpty() && message.encryption == Encryption::NoEncryption) {
		message.body = tr("This message is encrypted with %1 but could not be decrypted").arg(encryptionName);
	} else {
		// Do not use the file sharing fallback body.
		if (msg.body() != msg.outOfBandUrl()) {
			message.body = msg.body();
		}
	}

	message.isSpoiler = msg.isSpoiler();
	message.spoilerHint = msg.spoilerHint();
	message.outOfBandUrl = msg.outOfBandUrl();
	message.stanzaId = msg.stanzaId();
	message.originId = msg.originId();
	message.isMarkable = msg.isMarkable();

	// check if message contains a link and also check out of band url
	if (!parseMediaUri(message, msg.outOfBandUrl(), false)) {
		const QStringList bodyWords = message.body.split(u' ');
		for (const QString &word : bodyWords) {
			if (parseMediaUri(message, word, true))
				break;
		}
	}

	// get possible delay (timestamp)
	message.stamp = (msg.stamp().isNull() || !msg.stamp().isValid())
	                 ? QDateTime::currentDateTimeUtc()
	                 : msg.stamp().toUTC();

	// save the message to the database
	// in case of message correction, replace old message
	if (msg.replaceId().isEmpty()) {
		MessageDb::instance()->addMessage(message, origin);
	} else {
		message.isEdited = true;
		message.id.clear();
		MessageDb::instance()->updateMessage(msg.replaceId(), [message](Message &m) {
			// replace completely
			m = message;
		});
	}
}

void MessageHandler::sendMessage(const QString& toJid,
                                 const QString& body,
                                 bool isSpoiler,
                                 const QString& spoilerHint)
{
	Message msg;
	msg.from = AccountManager::instance()->jid();
	msg.to = toJid;
	msg.body = body;
	msg.id = QXmppUtils::generateStanzaUuid();
	msg.originId = msg.id;
	// MessageModel::activeEncryption() is thread-safe.
	msg.encryption = MessageModel::instance()->activeEncryption();
	msg.receiptRequested = true;
	msg.isOwn = true;
	msg.deliveryState = Enums::DeliveryState::Pending;
	msg.isMarkable = true;
	msg.stamp = QDateTime::currentDateTimeUtc();
	msg.isSpoiler = isSpoiler;
	msg.spoilerHint = spoilerHint;

	// process links from the body
	const QStringList words = body.split(u' ');
	for (const auto &word : words) {
		if (parseMediaUri(msg, word, true))
			break;
	}

	MessageDb::instance()->addMessage(msg, MessageOrigin::UserInput);
	sendPendingMessage(std::move(msg));
}

void MessageHandler::sendChatState(const QString &toJid, const QXmppMessage::State state)
{
	QXmppMessage message;
	message.setTo(toJid);
	message.setState(state);
	send(std::move(message));
}

void MessageHandler::sendCorrectedMessage(Message msg)
{
	const auto messageId = msg.id;
	await(send(toQXmppMessage(msg)), this, [messageId](QXmpp::SendResult result) {
		if (std::holds_alternative<QXmpp::SendError>(result)) {
			// TODO store in the database only error codes, assign text messages right in the QML
			emit Kaidan::instance()->passiveNotificationRequested(
						tr("Message correction was not successful"));

			MessageDb::instance()->updateMessage(messageId, [](Message &message) {
				message.deliveryState = DeliveryState::Error;
				message.errorText = QStringLiteral("Message correction was not successful");
			});
		} else {
			MessageDb::instance()->updateMessage(messageId, [](Message &message) {
				message.deliveryState = DeliveryState::Sent;
				message.errorText.clear();
			});
		}
	});
}

void MessageHandler::handleConnected()
{
	// retrieve missed messages, if the last saved message has been loaded and exists
	if (m_lastMessageLoaded && !m_lastMessageStamp.isNull()) {
		retrieveCatchUpMessages(m_lastMessageStamp);
	}
}

void MessageHandler::handleDisonnected()
{
	// clear all running backlog queries
	std::for_each(m_runningBacklogQueryIds.constKeyValueBegin(),
				  m_runningBacklogQueryIds.constKeyValueEnd(),
				  [this](const std::pair<QString, BacklogQueryState> &pair) {
		emit MessageModel::instance()->mamBacklogRetrieved(
				m_client->configuration().jidBare(), pair.second.chatJid, pair.second.lastTimestamp, false);
	});
	m_runningBacklogQueryIds.clear();

	m_runningInitialMessageQueryIds.clear();
	m_runnningCatchUpQueryId.clear();
}

void MessageHandler::sendPendingMessage(Message message)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		// if the message is a pending edition of the existing in the history message
		// I need to send it with the most recent stamp
		// for that I'm gonna copy that message and update in the copy just the stamp
		if (message.isEdited) {
			message.stamp = QDateTime::currentDateTimeUtc();
		}

		const auto messageId = message.id;
		await(send(toQXmppMessage(message)), this, [messageId](QXmpp::SendResult result) {
			if (const auto error = std::get_if<QXmpp::SendError>(&result)) {
				qWarning() << "[client] [MessageHandler] Could not send message:"
					<< error->text;

				// The error message of the message is saved untranslated. To make
				// translation work in the UI, the tr() call of the passive
				// notification must contain exactly the same string.
				emit Kaidan::instance()->passiveNotificationRequested(tr("Message could not be sent."));
				MessageDb::instance()->updateMessage(messageId, [](Message &msg) {
					msg.deliveryState = Enums::DeliveryState::Error;
					msg.errorText = QStringLiteral("Message could not be sent.");
				});
			} else {
				MessageDb::instance()->updateMessage(messageId, [](Message &msg) {
					msg.deliveryState = Enums::DeliveryState::Sent;
					msg.errorText.clear();
				});
			}
		});
	}
}

bool MessageHandler::parseMediaUri(Message &message, const QString &uri, bool isBodyPart)
{
	if (!MediaUtils::isHttp(uri) && !MediaUtils::isGeoLocation(uri)) {
		return false;
	}

	// check message type by file name in link
	// This is hacky, but needed without SIMS or an additional HTTP request.
	// Also, this can be useful when a user manually posts an HTTP url.
	const QUrl url(uri);
	const QMimeType mimeType = MediaUtils::mimeType(url);
	const MessageType messageType = MediaUtils::messageType(mimeType);

	switch (messageType) {
	case MessageType::MessageText:
	case MessageType::MessageUnknown:
		break;
	case MessageType::MessageFile:
		// Random files could be anything and could also include any website. We
		// want to avoid random links to be recognized as 'file'. Intentionally
		// sent files should be displayed of course.
		if (isBodyPart)
			break;
		[[fallthrough]];
	case MessageType::MessageGeoLocation:
		message.mediaLocation = url.toEncoded();
		[[fallthrough]];
	case MessageType::MessageImage:
	case MessageType::MessageAudio:
	case MessageType::MessageVideo:
	case MessageType::MessageDocument:
		message.outOfBandUrl = url.toEncoded();
		return true;
	}

	return false;
}

void MessageHandler::sendReadMarker(const QString &chatJid, const QString &messageId)
{
	Message message;
	message.to = chatJid;
	message.marker = QXmppMessage::Displayed;
	message.markerId = messageId;

	sendPendingMessage(message);
}

void MessageHandler::handlePendingMessages(const QVector<Message> &messages)
{
	for (Message message : messages) {
		sendPendingMessage(std::move(message));
	}
}

void MessageHandler::handleArchiveMessage(const QString &queryId,
                                          const QXmppMessage &message)
{
	if (queryId == m_runnningCatchUpQueryId) {
		handleMessage(message, MessageOrigin::MamCatchUp);
	} else if (m_runningInitialMessageQueryIds.contains(queryId)) {
		// TODO: request other message if this message is empty (e.g. no body)
		handleMessage(message, MessageOrigin::MamInitial);
	} else if (m_runningBacklogQueryIds.contains(queryId)) {
		handleMessage(message, MessageOrigin::MamBacklog);
		// update last stamp
		auto &lastTimestamp = m_runningBacklogQueryIds[queryId].lastTimestamp;
		if (lastTimestamp > message.stamp()) {
			lastTimestamp = message.stamp();
		}
	}
}

void MessageHandler::handleArchiveResults(const QString &queryId,
                                          const QXmppResultSetReply &,
                                          bool complete)
{
	if (queryId == m_runnningCatchUpQueryId) {
		m_runnningCatchUpQueryId.clear();
		Kaidan::instance()->database()->commitTransaction();
		return;
	}

	if (m_runningInitialMessageQueryIds.contains(queryId)) {
		m_runningInitialMessageQueryIds.removeOne(queryId);

		if (m_runningInitialMessageQueryIds.isEmpty()) {
			Kaidan::instance()->database()->commitTransaction();

			// so this won't be triggered again on reconnect
			m_lastMessageStamp = QDateTime::currentDateTimeUtc();
		}
		return;
	}

	if (m_runningBacklogQueryIds.contains(queryId)) {
		const auto state = m_runningBacklogQueryIds.take(queryId);
		emit MessageModel::instance()->mamBacklogRetrieved(m_client->configuration().jidBare(), state.chatJid, state.lastTimestamp, complete);
	}
}

void MessageHandler::retrieveInitialMessages()
{
	QXmppResultSetQuery queryLimit;
	// load only one message per user (the rest can be loaded when needed)
	queryLimit.setMax(1);
	// query last (newest) first
	queryLimit.setBefore("");

	const auto bareJids = m_client->findExtension<QXmppRosterManager>()->getRosterBareJids();
	if (bareJids.isEmpty()) {
		return;
	}

	m_runningInitialMessageQueryIds.clear();
	m_runningInitialMessageQueryIds.reserve(bareJids.size());

	for (const auto &jid : bareJids) {
		m_runningInitialMessageQueryIds.push_back(m_mamManager->retrieveArchivedMessages(
			QString(),
			QString(),
			jid,
			QDateTime(),
			QDateTime(),
			queryLimit
		));
	}

	Kaidan::instance()->database()->startTransaction();
}

void MessageHandler::retrieveCatchUpMessages(const QDateTime &stamp)
{
	QXmppResultSetQuery queryLimit;
	// no limit
	queryLimit.setMax(-1);

	m_runnningCatchUpQueryId = m_mamManager->retrieveArchivedMessages({}, {}, {}, stamp, {}, queryLimit);

	Kaidan::instance()->database()->startTransaction();
}

void MessageHandler::retrieveBacklogMessages(const QString &jid, const QDateTime &stamp)
{
	QXmppResultSetQuery queryLimit;
	queryLimit.setBefore("");
	queryLimit.setMax(MAM_BACKLOG_FETCH_COUNT);

	const auto id = m_mamManager->retrieveArchivedMessages({}, {}, jid, {}, stamp, queryLimit);
	m_runningBacklogQueryIds.insert(id, BacklogQueryState { jid, stamp });
}

bool MessageHandler::handleReadMarker(const QXmppMessage &message, const QString &senderJid, const QString &recipientJid, bool isOwnMessage)
{
	if (message.marker() == QXmppMessage::Displayed) {
		const auto markedId = message.markedId();
		if (isOwnMessage) {
			const auto lastReadContactMessageId = RosterModel::instance()->lastReadContactMessageId(senderJid, recipientJid);

			// Retrieve the count of messages between "lastReadContactMessageId" and "markedId" to
			// decrease the corresponding counter by 1 (if IDs could not be found) or by the actual
			// count of read messages.
			auto future = MessageDb::instance()->messageCount(recipientJid, senderJid, lastReadContactMessageId, markedId);
			await(future, this, [recipientJid, markedId](int count) {
				emit RosterModel::instance()->updateItemRequested(recipientJid, [=](RosterItem &item) {
					item.unreadMessages = count == 0 ? item.unreadMessages - 1 : item.unreadMessages - count + 1;
					item.lastReadContactMessageId = markedId;
				});
			});

			auto futureTimestamp = MessageDb::instance()->messageTimestamp(recipientJid, senderJid, markedId);
			await(futureTimestamp, this, [this, senderJid, recipientJid](QDateTime timestamp) {
				emit Notifications::instance()->closeMessageNotificationsRequested(senderJid, recipientJid, timestamp);
			});
		} else {
			emit RosterModel::instance()->updateItemRequested(senderJid, [markedId](RosterItem &item) {
				item.lastReadOwnMessageId = markedId;
			});

			emit MessageModel::instance()->updateLastReadOwnMessageIdRequested(recipientJid, senderJid);
		}

		return true;
	}

	return false;
}

QFuture<QXmpp::SendResult> MessageHandler::send(QXmppMessage &&message)
{
	switch (MessageModel::instance()->activeEncryption()) {
	case Encryption::NoEncryption:
		return m_client->sendUnencrypted(std::move(message));
	default:
		return m_client->send(std::move(message));
	}
}
