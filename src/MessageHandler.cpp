// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageHandler.h"
// std
// Qt
#include <QUrl>
#include <QRandomGenerator>
// QXmpp
#include <QXmppBitsOfBinaryContentId.h>
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppE2eeMetadata.h>
#include <QXmppEncryptedFileSource.h>
#include <QXmppFileMetadata.h>
#include <QXmppHash.h>
#include <QXmppHttpFileSource.h>
#include <QXmppMamManager.h>
#include <QXmppMessageReaction.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppRosterManager.h>
#include <QXmppThumbnail.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "AccountDb.h"
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
#include "RosterManager.h"
#include "RosterModel.h"

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
	connect(MessageModel::instance(), &MessageModel::sendCorrectedMessageRequested,
	        this, &MessageHandler::sendCorrectedMessage);
	connect(MessageModel::instance(), &MessageModel::sendChatStateRequested,
	        this, &MessageHandler::sendChatState);

	connect(client, &QXmppClient::connected, this, &MessageHandler::handleConnected);
	connect(client->findExtension<QXmppRosterManager>(), &QXmppRosterManager::rosterReceived,
	        this, &MessageHandler::handleRosterReceived);

	connect(&m_receiptManager, &QXmppMessageReceiptManager::messageDelivered,
		this, [](const QString &, const QString &id) {
		MessageDb::instance()->updateMessage(id, [](Message &msg) {
			msg.deliveryState = Enums::DeliveryState::Delivered;
			msg.errorText.clear();
		});
	});

	connect(this, &MessageHandler::retrieveBacklogMessagesRequested, this, &MessageHandler::retrieveBacklogMessages);

	client->addExtension(&m_receiptManager);
}

QFuture<QXmpp::SendResult> MessageHandler::send(QXmppMessage &&message)
{
	QFutureInterface<QXmpp::SendResult> interface(QFutureInterfaceBase::Started);

	const auto recipientJid = message.to();

	auto sendEncrypted = [=, this]() mutable {
		m_client->sendSensitive(std::move(message)).then(this, [=](QXmpp::SendResult result) mutable {
			reportFinishedResult(interface, result);
		});
	};

	auto sendUnencrypted = [=, this]() mutable {
		// Ensure that a message containing files but without a body is stored/archived via MAM by
		// the server.
		// That is not needed if the message is encrypted because that is handled by the
		// corresponding encryption manager.
		if (!message.sharedFiles().isEmpty() && message.body().isEmpty()) {
			message.addHint(QXmppMessage::Store);
		}

		m_client->send(std::move(message)).then(this, [=](QXmpp::SendResult result) mutable {
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
				if (const auto omemoEncryptionActive = activeEncryption == Encryption::Omemo2) {
					auto future = m_clientWorker->omemoManager()->hasUsableDevices({ recipientJid });
					await(future, this, [=](bool hasUsableDevices) mutable {
						const auto omemoEncryptionEnabled = omemoEncryptionActive && hasUsableDevices;

						if (omemoEncryptionEnabled) {
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

void MessageHandler::sendPendingMessages()
{
	auto future = MessageDb::instance()->fetchPendingMessages(AccountManager::instance()->jid());
	await(future, this, [this](QVector<Message> messages) {
		for (Message message : messages) {
			sendPendingMessage(std::move(message));
		}
	});
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
		if (std::holds_alternative<QXmppError>(result)) {
			// TODO store in the database only error codes, assign text messages right in the QML
			Q_EMIT Kaidan::instance()->passiveNotificationRequested(
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

QFuture<QXmpp::SendResult> MessageHandler::sendMessageReaction(const QString &chatJid, const QString &messageId, const QVector<QString> &emojis)
{
	QXmppMessageReaction reaction;
	reaction.setMessageId(messageId);
	reaction.setEmojis(emojis);

	QXmppMessage message;
	message.setTo(chatJid);
	message.setStamp(QDateTime::currentDateTimeUtc());
	message.addHint(QXmppMessage::Store);
	message.setReaction(reaction);
	message.setReceiptRequested(true);

	return send(std::move(message));
}

void MessageHandler::sendPendingMessage(Message message)
{
	if (m_client->state() == QXmppClient::ConnectedState) {
		// if the message is a pending edition of the existing in the history message
		// I need to send it with the most recent stamp
		// for that I'm gonna copy that message and update in the copy just the stamp
		if (!message.replaceId.isEmpty()) {
			message.timestamp = QDateTime::currentDateTimeUtc();
		}

		message.receiptRequested = true;

		const auto messageId = message.id;
		await(send(message.toQXmpp()), this, [messageId](QXmpp::SendResult result) {
			if (const auto error = std::get_if<QXmppError>(&result)) {
				qWarning() << "[client] [MessageHandler] Could not send message:"
				           << error->description;

				// The error message of the message is saved untranslated. To make
				// translation work in the UI, the tr() call of the passive
				// notification must contain exactly the same string.
				Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Message could not be sent."));
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
	QXmppMessage message;
	message.setTo(chatJid);
	message.setMarker(QXmppMessage::Displayed);
	message.setMarkerId(messageId);

	send(std::move(message));
}

void MessageHandler::handleConnected()
{
	// Fetch the stanza ID of the last message to retrieve all new messages from the server since
	// then if the there is such a last message locally stored.
	// That is not the case when the user's account is added to Kaidan.
	await(AccountDb::instance()->fetchLatestMessageStanzaId(m_client->configuration().jidBare()), this, [this](QString &&stanzaId) {
		if (!stanzaId.isEmpty()) {
			retrieveCatchUpMessages(stanzaId);
		}
	});
}

void MessageHandler::handleRosterReceived()
{
	// Fetch the stanza ID of the last message to retrieve one initial message for each roster item
	// from the server if there is no last message locally stored.
	// That is the case when the user's account is added to Kaidan.
	await(AccountDb::instance()->fetchLatestMessageStanzaId(m_client->configuration().jidBare()), this, [this](QString &&stanzaId) {
		if (stanzaId.isEmpty()) {
			retrieveInitialMessages();
		}
	});
}

void MessageHandler::retrieveInitialMessages()
{
	const auto jids = m_client->findExtension<QXmppRosterManager>()->getRosterBareJids();
	if (jids.isEmpty()) {
		return;
	}

	// start database transaction if no queries are running and we are going to request messages
	if (!m_runningInitialMessageQueries && !jids.empty()) {
		Kaidan::instance()->database()->startTransaction();
	}

	for (const auto &jid : jids) {
		m_runningInitialMessageQueries++;
		retrieveInitialMessage(jid);
	}
}

void MessageHandler::retrieveInitialMessage(const QString &jid, const QString &offsetMessageId)
{
	using Mam = QXmppMamManager;

	QXmppResultSetQuery queryLimit;
	queryLimit.setMax(1);
	queryLimit.setBefore(offsetMessageId);

	m_mamManager->retrieveMessages(
		QString(),
		QString(),
		jid,
		QDateTime(),
		QDateTime(),
		queryLimit).then(this, [this, jid](Mam::RetrieveResult result) {

		// process received message
		if (auto retrievedMessages = std::get_if<Mam::RetrievedMessages>(&result)) {
			if (const auto messages = retrievedMessages->messages; !messages.empty()) {
				const auto message = messages.constFirst();

				if (message.body().isEmpty() && message.sharedFiles().isEmpty()) {
					retrieveInitialMessage(jid, retrievedMessages->result.resultSetReply().first());
					return;
				}

				handleMessage(message, MessageOrigin::MamInitial);
			}
		}
		// do nothing on error

		// commit database transaction when all queries have finished
		if (!--m_runningInitialMessageQueries) {
			Kaidan::instance()->database()->commitTransaction();
		}
	});
}

void MessageHandler::retrieveCatchUpMessages(const QString &latestMessageStanzaId)
{
	using Mam = QXmppMamManager;

	QXmppResultSetQuery queryLimit;

	queryLimit.setAfter(latestMessageStanzaId);
	// no limit
	queryLimit.setMax(-1);

	m_mamManager->retrieveMessages({}, {}, {}, {}, {}, queryLimit).then(this, [this](auto result) {
		if (std::holds_alternative<typename Mam::RetrievedMessages>(result)) {
			auto messages = std::get<typename Mam::RetrievedMessages>(std::move(result));

			// process messages
			Kaidan::instance()->database()->startTransaction();
			for (const auto &message : std::as_const(messages.messages)) {
				handleMessage(message, MessageOrigin::MamCatchUp);

				// Send delivery receipts for catched up messages.
				m_receiptManager.handleMessage(message);
			}
			Kaidan::instance()->database()->commitTransaction();
		}
		if (auto *err = std::get_if<QXmppError>(&result)) {
			qDebug() << "[MAM] Error while fetching catch-up messages:" << err->description;
		}
	});
}

void MessageHandler::retrieveBacklogMessages(const QString &jid, const QDateTime &stamp)
{
	// TODO: Return QFuture/QXmppTask here instead of emitting signal in MessageModel
	using Mam = QXmppMamManager;

	QXmppResultSetQuery queryLimit;
	queryLimit.setBefore("");
	queryLimit.setMax(MAM_BACKLOG_FETCH_COUNT);

	m_mamManager->retrieveMessages({}, {}, jid, {}, stamp, queryLimit).then(this, [this, jid, stamp](auto result) {
		const auto ownJid = m_client->configuration().jidBare();
		if (std::holds_alternative<typename Mam::RetrievedMessages>(result)) {
			auto messages = std::get<typename Mam::RetrievedMessages>(std::move(result));
			auto lastTimestamp = [&]() {
				if (messages.messages.empty()) {
					return stamp;
				}
				// requires ranges support
				// return std::ranges::max(messages.messages, {}, &QXmppMessage::stamp).stamp();
				return std::max_element(messages.messages.begin(), messages.messages.end(), [](const auto &a, const auto &b) {
					return a.stamp() < b.stamp();
				})->stamp();
			}();

			// TODO: Do real batch processing (especially in the DB)
			Kaidan::instance()->database()->startTransaction();
			for (const auto &message : std::as_const(messages.messages)) {
				handleMessage(message, MessageOrigin::MamBacklog);
			}
			Kaidan::instance()->database()->commitTransaction();

			Q_EMIT MessageModel::instance()->mamBacklogRetrieved(ownJid, jid, lastTimestamp, messages.result.complete());

		} else if (auto err = std::get_if<QXmppError>(&result)) {
			qDebug() << "[MAM]" << "Error requesting MAM backlog:" << err->description;
			Q_EMIT MessageModel::instance()->mamBacklogRetrieved(ownJid, jid, stamp, false);
		}
	});
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

	const auto accountJid = m_client->configuration().jidBare();
	const auto senderJid = QXmppUtils::jidToBareJid(msg.from());
	const auto recipientJid = QXmppUtils::jidToBareJid(msg.to());

	Message message;
	message.accountJid = accountJid;
	message.senderId = senderJid;
	message.chatJid = message.isOwn() ? recipientJid : senderJid;

	if (msg.state() != QXmppMessage::State::None) {
		Q_EMIT MessageModel::instance()->handleChatStateRequested(senderJid, msg.state());
	}

	if (handleReadMarker(msg, senderJid, recipientJid, message.isOwn())) {
		return;
	}

	if (handleReaction(msg, senderJid)) {
		return;
	}

	if (msg.body().isEmpty() && msg.outOfBandUrl().isEmpty() && msg.sharedFiles().isEmpty()) {
		return;
	}

	// Close a notification for messages to which the user replied via another own resource.
	if (message.isOwn()) {
		Q_EMIT Notifications::instance()->closeMessageNotificationRequested(senderJid, recipientJid);
	}

	message.id = msg.id();

	if (auto e2eeMetadata = msg.e2eeMetadata()) {
		message.encryption = Encryption::Enum(e2eeMetadata->encryption());
		message.senderKey = e2eeMetadata->senderKey();
	}

	parseSharedFiles(msg, message);

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

	AccountDb::instance()->updateAccount(accountJid, [stanzaId = msg.stanzaId(), timestamp = msg.stamp().toUTC()](Account &account) {
		// Check "<=" instead of "<" to update the ID in the rare case that the timestamps are equal
		// (e.g., because the timestamps are not precise enough)
		if (account.latestMessageTimestamp <= timestamp) {
			account.latestMessageStanzaId = stanzaId;
			account.latestMessageTimestamp = timestamp;
		}
	});

	// save the message to the database
	// in case of message correction, replace old message
	if (msg.replaceId().isEmpty()) {
		// get possible delay (timestamp)
		message.timestamp = (msg.stamp().isNull() || !msg.stamp().isValid())
						 ? QDateTime::currentDateTimeUtc()
						 : msg.stamp().toUTC();

		MessageDb::instance()->addMessage(message, origin);

		// Add the message's sender to the roster if not already done and only for direct messages.
		// Otherwise, the chat could only be opened via the message's notification and could not be
		// opened again later.
		if (msg.type() != QXmppMessage::GroupChat && !RosterModel::instance()->hasItem(senderJid)) {
			m_clientWorker->rosterManager()->addContact(senderJid);
		}
	} else {
		const auto replaceId = msg.replaceId();
		message.replaceId = replaceId;
		MessageDb::instance()->updateMessage(replaceId, [message](Message &m) {
			// Replace the whole stored message but keep its original timestamp.
			const auto timestamp = m.timestamp;
			m = message;
			m.timestamp = timestamp;
		});
	}
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
				Q_EMIT RosterModel::instance()->updateItemRequested(recipientJid, [=](RosterItem &item) {
					item.unreadMessages = count == 0 ? item.unreadMessages - 1 : item.unreadMessages - count + 1;
					item.lastReadContactMessageId = markedId;
					item.readMarkerPending = false;
				});
			});

			Q_EMIT Notifications::instance()->closeMessageNotificationRequested(senderJid, recipientJid);
		} else {
			Q_EMIT RosterModel::instance()->updateItemRequested(senderJid, [markedId](RosterItem &item) {
				item.lastReadOwnMessageId = markedId;
			});

			runOnThread(MessageModel::instance(), []() {
				MessageModel::instance()->updateLastReadOwnMessageId();
			});
		}

		return true;
	}

	return false;
}

bool MessageHandler::handleReaction(const QXmppMessage &message, const QString &senderJid)
{
	if (const auto receivedReaction = message.reaction()) {
		MessageDb::instance()->updateMessage(message.reaction()->messageId(), [senderJid, receivedEmojis = receivedReaction->emojis(), receivedTimestamp = message.stamp()](Message &m) {
			auto &reactionSenders = m.reactionSenders;
			auto &reactionSender = reactionSenders[senderJid];

			// Process only newer reactions.
			if (reactionSender.latestTimestamp.isNull() || reactionSender.latestTimestamp < receivedTimestamp) {
				reactionSender.latestTimestamp = receivedTimestamp;
				auto &reactions = reactionSender.reactions;

				// Add new reactions.
				for (const auto &receivedEmoji : std::as_const(receivedEmojis)) {
					const auto reactionNew = std::none_of(reactions.begin(), reactions.end(), [&](const MessageReaction &reaction) {
						return reaction.emoji == receivedEmoji;
					});

					if (reactionNew) {
						MessageReaction reaction;
						reaction.emoji = receivedEmoji;

						reactionSender.latestTimestamp = QDateTime::currentDateTimeUtc();
						reactions.append(reaction);
					}
				}

				// Remove existing reactions.
				for (auto itr = reactions.begin(); itr != reactions.end();) {
					if (!receivedEmojis.contains(itr->emoji)) {
						reactions.erase(itr);
					} else {
						++itr;
					}
				}

				// Remove the reaction sender if it has no reactions anymore.
				if (reactions.isEmpty()) {
					reactionSenders.remove(senderJid);
				}
			}
		});

		return true;
	}

	return false;
}

void MessageHandler::parseSharedFiles(const QXmppMessage &message, Message &messageToEdit)
{
	if (const auto sharedFiles = message.sharedFiles(); !sharedFiles.empty()) {
		messageToEdit.fileGroupId = QRandomGenerator::system()->generate64();
		messageToEdit.files = transform(sharedFiles, [message, fgid = messageToEdit.fileGroupId](const QXmppFileShare &file) {
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
					const auto &bobData = message.bitsOfBinaryData();
					if (!file.metadata().thumbnails().empty()) {
						auto cid = QXmppBitsOfBinaryContentId::fromCidUrl(file.metadata().thumbnails().front().uri());
						const auto *thumbnailData = std::find_if(bobData.begin(), bobData.end(), [&](auto bobBlob) {
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
	} else if (auto urls = message.outOfBandUrls(); !urls.isEmpty()) {
		const qint64 fileGroupId = QRandomGenerator::system()->generate64();
		messageToEdit.files = transformFilter(urls, [&](auto &file) {
			return MessageHandler::parseOobUrl(file, fileGroupId);
		});

		// don't set file group id if there are no files
		if (!messageToEdit.files.empty()) {
			messageToEdit.fileGroupId = fileGroupId;
		}
	}
}
