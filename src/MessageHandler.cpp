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
#include <QRandomGenerator>
// QXmpp
#include "QXmppBitsOfBinaryContentId.h"
#include "QXmppBitsOfBinaryDataList.h"
#include <QXmppE2eeMetadata.h>
#include "QXmppEncryptedFileSource.h"
#include <QXmppFileMetadata.h>
#include <QXmppHash.h>
#include "QXmppHttpFileSource.h"
#include <QXmppMamManager.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppRosterManager.h>
#include <QXmppThumbnail.h>
#include <QXmppUtils.h>
// Kaidan
#include "AccountManager.h"
#include "Algorithms.h"
#include "ClientWorker.h"
#include "Database.h"
#include "FutureUtils.h"
#include "Kaidan.h"
#include "Message.h"
#include "MessageDb.h"
#include "MessageModel.h"
#include "MediaUtils.h"
#include "Notifications.h"
#include "OmemoManager.h"
#include "RosterModel.h"

namespace ranges = std::ranges;

// Number of messages fetched at once when loading MAM backlog
constexpr int MAM_BACKLOG_FETCH_COUNT = 40;

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
		this, [](const QString &, const QString &id) {
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

	if (const auto sharedFiles = msg.sharedFiles(); !sharedFiles.empty()) {
		message.fileGroupId = QRandomGenerator::system()->generate64();
		message.files = transform(sharedFiles, [msg, fgid = message.fileGroupId](const QXmppFileShare &file) {
			auto fileId = qint64(QRandomGenerator::system()->generate64());
			return File {
				.id = fileId,
				.fileGroupId = fgid.value(),
				.name = file.metadata().filename(),
				.description = file.metadata().description().value_or(QString()),
				.mimeType = file.metadata().mediaType().value_or(QMimeType()),
				.size = file.metadata().size(),
				.lastModified = file.metadata().lastModified().value_or(QDateTime()),
				.disposition = file.disposition(),
				.localFilePath = {},
				.hashes = transform(file.metadata().hashes(), [&](const QXmppHash &hash) {
					return FileHash {
						.dataId = fileId,
						.hashType = hash.algorithm(),
						.hashValue = hash.hash()
					};
				}),
				.thumbnail = [&]() {
					const auto &bobData = msg.bitsOfBinaryData();
					if (!file.metadata().thumbnails().empty()) {
						auto cid = QXmppBitsOfBinaryContentId::fromCidUrl(file.metadata().thumbnails().front().uri());
						const auto *thumbnailData = ranges::find_if(bobData, [&](auto bobBlob) {
							return bobBlob.cid() == cid;
						});

						if (thumbnailData != bobData.cend()) {
							return thumbnailData->data();
						}
					}
					return QByteArray();
				}(),
				.httpSources = transform(file.httpSources(), [&](const auto &source) {
					return HttpSource { fileId, source.url() };
				}),
				.encryptedSources = transformFilter(file.encryptedSources(), [&](const QXmppEncryptedFileSource &source) -> std::optional<EncryptedSource> {
					if (source.httpSources().empty()) {
						return {};
					}
					std::optional<qint64> encryptedDataId;
					if (!source.hashes().empty()) {
						encryptedDataId = QRandomGenerator::system()->generate64();
					}
					return EncryptedSource {
						fileId,
						source.httpSources().front().url(),
						source.cipher(),
						source.key(),
						source.iv(),
						encryptedDataId,
						transform(source.hashes(), [&](const QXmppHash &h) {
							return FileHash { *encryptedDataId, h.algorithm(), h.hash() };
						})
					};
				}),
			};
		});
	} else if (auto urls = msg.outOfBandUrls(); !urls.isEmpty()) {
		const qint64 fileGroupId = QRandomGenerator::system()->generate64();
		message.files = transformFilter(urls, [&](auto &file) {
			return MessageHandler::parseOobUrl(file, fileGroupId);
		});

		// don't set file group id if there are no files
		if (!message.files.empty()) {
			message.fileGroupId = fileGroupId;
		}
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
	message.stanzaId = msg.stanzaId();
	message.originId = msg.originId();
	message.isMarkable = msg.isMarkable();

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
	await(send(msg.toQXmpp()), this, [messageId](QXmpp::SendResult result) {
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
		await(send(message.toQXmpp()), this, [messageId](QXmpp::SendResult result) {
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

std::optional<File> MessageHandler::parseOobUrl(const QXmppOutOfBandUrl &url, qint64 fileGroupId)
{
	if (!MediaUtils::isHttp(url.url())) {
		return {};
	}

	// TODO: consider doing a HEAD request to fill in the remaining metadata

	const auto name = QUrl(url.url()).fileName();
	const auto id = static_cast<qint64>(QRandomGenerator::system()->generate64());
	return File {
		.id = id,
		.fileGroupId = fileGroupId,
		.name = name,
		.description = url.description(),
		.mimeType = [&name] {
			const auto possibleMimeTypes = MediaUtils::mimeDatabase().mimeTypesForFileName(name);
			if (possibleMimeTypes.empty()) {
				return MediaUtils::mimeDatabase().mimeTypeForName("application/octet-stream");
			}

			return possibleMimeTypes.front();
		}(),
		.size = {},
		.lastModified = {},
		.localFilePath = {},
		.hashes = {},
		.thumbnail = {},
		.httpSources = {
			HttpSource {
				.fileId = id,
				.url = url.url()
			}
		},
		.encryptedSources = {}
	};
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
			await(futureTimestamp, this, [senderJid, recipientJid](QDateTime timestamp) {
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
	QFutureInterface<QXmpp::SendResult> interface(QFutureInterfaceBase::Started);

	const auto recipientJid = message.to();

	auto sendEncrypted = [=, this]() mutable {
		await(m_client->send(std::move(message)), this, [=](QXmpp::SendResult result) mutable {
			reportFinishedResult(interface, result);
		});
	};

	auto sendUnencrypted = [=, this]() mutable {
		await(m_client->sendUnencrypted(std::move(message)), this, [=](QXmpp::SendResult result) mutable {
			reportFinishedResult(interface, result);
		});
	};

	// If the message is sent for the current chat, its information is used to determine whether to
	// send encrypted.
	// Otherwise, that information is retrieved from the database.
	runOnThread(MessageModel::instance(), [accountJid = AccountManager::instance()->jid(), recipientJid]() {
		return MessageModel::instance()->isChatCurrentChat(accountJid, recipientJid);
	}, this, [=, this](bool isChatCurrentChat) mutable {
		if (isChatCurrentChat) {
			runOnThread(MessageModel::instance(), []() {
				return MessageModel::instance()->isOmemoEncryptionEnabled();
			}, this, [=](bool isOmemoEncryptionEnabled) mutable {
				if (isOmemoEncryptionEnabled) {
					sendEncrypted();
				} else {
					sendUnencrypted();
				}
			});
		} else {
			runOnThread(RosterModel::instance(), [accountJid = AccountManager::instance()->jid(), recipientJid]() {
				return RosterModel::instance()->itemEncryption(accountJid, recipientJid).value_or(Encryption::NoEncryption);
			}, this, [=, this](Encryption::Enum activeEncryption) mutable {
				if (activeEncryption == Encryption::Omemo2) {
					auto future = m_clientWorker->omemoManager()->hasUsableDevices({ recipientJid });
					await(future, this, [=](bool hasUsableDevices) mutable {
						const auto isOmemoEncryptionEnabled = activeEncryption == Encryption::Omemo2 && hasUsableDevices;

						if (isOmemoEncryptionEnabled) {
							sendEncrypted();
						} else {
							sendUnencrypted();
						}
					});
				} else {
					sendUnencrypted();
				}
			});
		}
	});

	return interface.future();
}
