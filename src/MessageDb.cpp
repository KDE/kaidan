// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageDb.h"

// Qt
#include <QSqlDatabase>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlRecord>
#include <QStringBuilder>
#include <QMimeDatabase>
#include <QBuffer>
#include <QFile>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Algorithms.h"
#include "Database.h"
#include "Globals.h"

Q_DECLARE_METATYPE(QXmpp::Cipher)
Q_DECLARE_METATYPE(QXmpp::HashAlgorithm)
Q_DECLARE_METATYPE(QXmppFileShare::Disposition)

#define CHECK_MESSAGE_EXISTS_DEPTH_LIMIT "20"

template<typename T>
QVariant optionalToVariant(std::optional<T> value)
{
	if (value) {
		return QVariant(*value);
	}
	return {};
}

template<typename T>
std::optional<T> variantToOptional(QVariant value)
{
	// ## Qt6 ## Does isNull() also work with Qt 6?
	if (!value.isNull() && value.canConvert<T>()) {
		return value.value<T>();
	}
	return {};
}

MessageDb *MessageDb::s_instance = nullptr;

MessageDb::MessageDb(Database *db, QObject *parent)
	: DatabaseComponent(db, parent)
{
	Q_ASSERT(!MessageDb::s_instance);
	s_instance = this;
}

MessageDb::~MessageDb()
{
	s_instance = nullptr;
}

MessageDb *MessageDb::instance()
{
	return s_instance;
}

QVector<Message> MessageDb::_fetchMessagesFromQuery(QSqlQuery &query)
{
	QVector<Message> messages;

	// get indexes of attributes
	QSqlRecord rec = query.record();
	int idxAccountJid = rec.indexOf(QStringLiteral("accountJid"));
	int idxChatJid = rec.indexOf(QStringLiteral("chatJid"));
	int idxSenderId = rec.indexOf(QStringLiteral("senderId"));
	int idxId = rec.indexOf(QStringLiteral("id"));
	int idxOriginId = rec.indexOf(QStringLiteral("originId"));
	int idxStanzaId = rec.indexOf(QStringLiteral("stanzaId"));
	int idxReplaceId = rec.indexOf(QStringLiteral("replaceId"));
	int idxTimestamp = rec.indexOf(QStringLiteral("timestamp"));
	int idxBody = rec.indexOf(QStringLiteral("body"));
	int idxEncryption = rec.indexOf(QStringLiteral("encryption"));
	int idxSenderKey = rec.indexOf(QStringLiteral("senderKey"));
	int idxDeliveryState = rec.indexOf(QStringLiteral("deliveryState"));
	int idxIsSpoiler = rec.indexOf(QStringLiteral("isSpoiler"));
	int idxSpoilerHint = rec.indexOf(QStringLiteral("spoilerHint"));
	int idxFileGroupId = rec.indexOf(QStringLiteral("fileGroupId"));
	int idxErrorText = rec.indexOf(QStringLiteral("errorText"));
	int idxRemoved = rec.indexOf(QStringLiteral("removed"));

	reserve(messages, query);
	while (query.next()) {
		Message msg;
		msg.accountJid = query.value(idxAccountJid).toString();
		msg.chatJid = query.value(idxChatJid).toString();
		msg.senderId = query.value(idxSenderId).toString();
		msg.id = query.value(idxId).toString();
		msg.originId = query.value(idxOriginId).toString();
		msg.stanzaId = query.value(idxStanzaId).toString();
		msg.replaceId = query.value(idxReplaceId).toString();
		msg.timestamp = QDateTime::fromString(
			query.value(idxTimestamp).toString(),
			Qt::ISODate
		);
		msg.body = query.value(idxBody).toString();
		msg.encryption = query.value(idxEncryption).value<Encryption::Enum>();
		msg.senderKey = query.value(idxSenderKey).toByteArray();
		msg.deliveryState = query.value(idxDeliveryState).value<Enums::DeliveryState>();
		msg.isSpoiler = query.value(idxIsSpoiler).toBool();
		msg.spoilerHint = query.value(idxSpoilerHint).toString();
		msg.fileGroupId = variantToOptional<qint64>(query.value(idxFileGroupId));
		if (msg.fileGroupId) {
			msg.files = _fetchFiles(*msg.fileGroupId);
		}
		msg.errorText = query.value(idxErrorText).toString();
		msg.removed = query.value(idxRemoved).toBool();

		messages << std::move(msg);
	}
	return messages;
}

QSqlRecord MessageDb::createUpdateRecord(const Message &oldMsg, const Message &newMsg)
{
	QSqlRecord rec;

	if (oldMsg.accountJid != newMsg.accountJid) {
		rec.append(createSqlField(QStringLiteral("accountJid"), newMsg.accountJid));
	}
	if (oldMsg.chatJid != newMsg.chatJid) {
		rec.append(createSqlField(QStringLiteral("chatJid"), newMsg.chatJid));
	}
	if (oldMsg.senderId != newMsg.senderId) {
		rec.append(createSqlField(QStringLiteral("senderId"), newMsg.senderId));
	}
	if (oldMsg.id != newMsg.id) {
		rec.append(createSqlField(QStringLiteral("id"), newMsg.id));
	}
	if (oldMsg.originId != newMsg.originId) {
		rec.append(createSqlField(QStringLiteral("originId"), newMsg.originId));
	}
	if (oldMsg.stanzaId != newMsg.stanzaId) {
		rec.append(createSqlField(QStringLiteral("stanzaId"), newMsg.stanzaId));
	}
	if (oldMsg.replaceId != newMsg.replaceId) {
		rec.append(createSqlField(QStringLiteral("replaceId"), newMsg.replaceId));
	}
	if (oldMsg.timestamp != newMsg.timestamp) {
		rec.append(createSqlField(QStringLiteral("timestamp"), newMsg.timestamp.toString(Qt::ISODateWithMs)));
	}
	if (oldMsg.body != newMsg.body) {
		rec.append(createSqlField(QStringLiteral("body"), newMsg.body));
	}
	if (oldMsg.encryption != newMsg.encryption) {
		rec.append(createSqlField(QStringLiteral("encryption"), newMsg.encryption));
	}
	if (oldMsg.senderKey != newMsg.senderKey) {
		rec.append(createSqlField(QStringLiteral("senderKey"), newMsg.senderKey));
	}
	if (oldMsg.deliveryState != newMsg.deliveryState) {
		rec.append(createSqlField(QStringLiteral("deliveryState"), int(newMsg.deliveryState)));
	}
	if (oldMsg.isSpoiler != newMsg.isSpoiler) {
		rec.append(createSqlField(QStringLiteral("isSpoiler"), newMsg.isSpoiler));
	}
	if (oldMsg.spoilerHint != newMsg.spoilerHint) {
		rec.append(createSqlField(QStringLiteral("spoilerHint"), newMsg.spoilerHint));
	}
	if (oldMsg.fileGroupId != newMsg.fileGroupId) {
		rec.append(createSqlField(QStringLiteral("fileGroupId"), optionalToVariant(newMsg.fileGroupId)));
	}
	if (oldMsg.errorText != newMsg.errorText) {
		rec.append(createSqlField(QStringLiteral("errorText"), newMsg.errorText));
	}
	if (oldMsg.removed != newMsg.removed) {
		rec.append(createSqlField(QStringLiteral("removed"), newMsg.removed));
	}

	return rec;
}

QFuture<QVector<Message>> MessageDb::fetchMessages(const QString &accountJid, const QString &chatJid, int index)
{
	return run([this, accountJid, chatJid, index]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid
				ORDER BY timestamp DESC
				LIMIT :index, :limit
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
				{ u":limit", DB_QUERY_LIMIT_MESSAGES },
			}
		);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		Q_EMIT messagesFetched(messages);
		return messages;
	});
}

QFuture<QVector<File>> MessageDb::fetchFiles(const QString &accountJid)
{
	return run([this, accountJid]() {
		return _fetchFiles(accountJid);
	});
}

QFuture<QVector<File>> MessageDb::fetchFiles(const QString &accountJid, const QString &chatJid)
{
	return run([this, accountJid, chatJid]() {
		return _fetchFiles(accountJid, chatJid);
	});
}

QFuture<QVector<File>> MessageDb::fetchDownloadedFiles(const QString &accountJid)
{
	return run([this, accountJid]() {
		auto files = _fetchFiles(accountJid);
		_extractDownloadedFiles(files);

		return files;
	});
}

QFuture<QVector<File>> MessageDb::fetchDownloadedFiles(const QString &accountJid, const QString &chatJid)
{
	return run([this, accountJid, chatJid]() {
		auto files = _fetchFiles(accountJid, chatJid);
		_extractDownloadedFiles(files);

		return files;
	});
}

QFuture<QVector<Message> > MessageDb::fetchMessagesUntilFirstContactMessage(const QString &accountJid, const QString &chatJid, int index)
{
	return run([this, accountJid, chatJid, index]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid
				ORDER BY timestamp DESC
				LIMIT
					:index,
					:limit + (
						SELECT COUNT()
						FROM chatMessages
						WHERE
							accountJid = :accountJid AND chatJid = :chatJid AND
							timestamp >= (
								SELECT timestamp
								FROM chatMessages
								WHERE senderId = :chatJid
							)
					)
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
				{ u":limit", DB_QUERY_LIMIT_MESSAGES },
			}
		);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		Q_EMIT messagesFetched(messages);
		return messages;
	});
}

QFuture<QVector<Message>> MessageDb::fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId)
{
	return run([this, accountJid, chatJid, index, limitingId]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid
				ORDER BY timestamp DESC
				LIMIT
					:index,
					:limit + (
						SELECT COUNT()
						FROM chatMessages
						WHERE timestamp >= (
							SELECT timestamp
							FROM chatMessages
							WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :id
						)
					)
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
				{ u":limit", DB_QUERY_LIMIT_MESSAGES },
				{ u":id", limitingId },
			}
		);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		Q_EMIT messagesFetched(messages);
		return messages;
	});
}

QFuture<MessageDb::MessageResult> MessageDb::fetchMessagesUntilQueryString(const QString &accountJid, const QString &chatJid, int index, const QString &queryString)
{
	return run([this, accountJid, chatJid, index, queryString]() -> MessageResult {
		auto query = createQuery();

		execQuery(
			query,
			QStringLiteral(R"(
				SELECT COUNT()
				FROM chatMessages
				WHERE
					accountJid = :accountJid AND chatJid = :chatJid AND
					timestamp >= (
						SELECT timestamp
						FROM chatMessages
						WHERE
							accountJid = :accountJid AND chatJid = :chatJid AND
							body LIKE :queryString AND
							timestamp <= (
								SELECT timestamp
								FROM chatMessages
								WHERE accountJid = :accountJid AND chatJid = :chatJid
								ORDER BY timestamp DESC
								LIMIT :index, 1
							)
						ORDER BY timestamp DESC
						LIMIT 1
					)
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
				// '%' is intended here as a placeholder inside the query for SQL statement "LIKE".
				{ u":queryString", QString(QLatin1Char('%') + queryString + QLatin1Char('%')) },
			}
		);

		query.first();
		const auto queryStringMessageIndex = query.value(0).toInt();
		const auto messagesUntilQueryStringCount = queryStringMessageIndex - index;

		// Skip further processing if no message with queryString could be found.
		if (messagesUntilQueryStringCount <= 0) {
			return {};
		}

		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid
				ORDER BY timestamp DESC
				LIMIT :index, :limit
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
				{ u":limit", messagesUntilQueryStringCount + DB_QUERY_LIMIT_MESSAGES },
			}
		);

		MessageResult result {
			_fetchMessagesFromQuery(query),
			// Database index starts at 1, but message model index starts at 0.
			queryStringMessageIndex - 1
		};

		_fetchReactions(result.messages);

		Q_EMIT messagesFetched(result.messages);

		return result;
	});
}

Message MessageDb::_fetchLastMessage(const QString &accountJid, const QString &chatJid)
{
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			SELECT *
			FROM messages
			WHERE accountJid = :accountJid AND chatJid = :chatJid AND removed = 0
			ORDER BY timestamp DESC
			LIMIT 1
		)"),
		{
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
		}
	);

	auto messages = _fetchMessagesFromQuery(query);

	if (!messages.isEmpty()) {
		return messages.first();
	}

	return {};
}

QFuture<QString> MessageDb::firstContactMessageId(const QString &accountJid, const QString &chatJid, int index)
{
	return run([=, this]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT id
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid AND senderId = :chatJid
				ORDER BY timestamp DESC
				LIMIT :index, 1
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":index", index },
			}
		);

		if (query.first()) {
			return query.value(0).toString();
		}

		return QString();
	});
}

QFuture<int> MessageDb::messageCount(const QString &accountJid, const QString &chatJid, const QString &messageIdBegin, const QString &messageIdEnd)
{
	return run([=, this]() {
		auto query = createQuery();
		execQuery(
			query,
			// The double round brackets are needed to evaluate the datetime.
			QStringLiteral(R"(
				SELECT COUNT(*)
				FROM chatMessages DESC
				WHERE
					accountJid = :accountJid AND chatJid = :chatJid AND
					datetime(timestamp) BETWEEN
						datetime((
							SELECT timestamp
							FROM chatMessages DESC
							WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageIdBegin
							LIMIT 1
						)) AND
						datetime((
							SELECT timestamp
							FROM chatMessages DESC
							WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageIdEnd
							LIMIT 1
						))
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":messageIdBegin", messageIdBegin },
				{ u":messageIdEnd", messageIdEnd },
			}
		);

		if (query.first()) {
			return query.value(0).toInt();
		}

		return 0;
	});
}

QFuture<void> MessageDb::addMessage(const Message &msg, MessageOrigin origin)
{
	Q_ASSERT(msg.deliveryState != DeliveryState::Draft);

	return run([this, msg, origin]() {
		// deduplication
		switch (origin) {
		case MessageOrigin::MamBacklog:
		case MessageOrigin::MamCatchUp:
		case MessageOrigin::Stream:
			if (_checkMessageExists(msg)) {
				// Mark messages sent to oneself as delivered.
				if (msg.isOwn() && msg.accountJid == msg.chatJid) {
					updateMessage(msg.id, [](Message &msg) {
						msg.deliveryState = Enums::DeliveryState::Delivered;
					});
				}

				// message deduplicated (messageAdded() signal is not emitted)
				return;
			}
			break;
		case MessageOrigin::MamInitial:
		case MessageOrigin::UserInput:
			// no deduplication required
			break;
		}

		// to speed up the whole process emit signal first and do the actual insert after that
		Q_EMIT messageAdded(msg, origin);

		_addMessage(msg);
		_setFiles(msg.files);
	});
}

QFuture<void> MessageDb::removeAllMessagesFromAccount(const QString &accountJid)
{
	return run([this, accountJid]() {
		auto query = createQuery();

		// remove files
		{
			execQuery(
				query,
				QStringLiteral(R"(
					SELECT fileGroupId
					FROM messages
					WHERE accountJid = :accountJid AND fileGroupId IS NOT NULL
				)"),
				{
					{ u":accountJid", accountJid },
				}
			);

			QVector<qint64> fileIds;
			reserve(fileIds, query);
			while (query.next()) {
				fileIds.append(query.value(0).toLongLong());
			}
			if (!fileIds.isEmpty()) {
				_removeFiles(fileIds);
				_removeFileHashes(fileIds);
			}
		}

		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM messages WHERE accountJid = :accountJid
			)"),
			{
				{ u":accountJid", accountJid },
			}
		);

		allMessagesRemovedFromAccount(accountJid);
	});
}

QFuture<void> MessageDb::removeAllMessagesFromChat(const QString &accountJid, const QString &chatJid)
{
	return run([this, accountJid, chatJid]() {
		auto query = createQuery();

		// remove files
		{
			execQuery(
				query,
				QStringLiteral(R"(
					SELECT fileGroupId
					FROM messages
					WHERE accountJid = :accountJid AND chatJid = :chatJid AND fileGroupId IS NOT NULL
				)"),
				{
					{ u":accountJid", accountJid },
					{ u":chatJid", chatJid },
				}
			);

			QVector<qint64> fileIds;
			reserve(fileIds, query);
			while (query.next()) {
				fileIds.append(query.value(0).toLongLong());
			}
			if (!fileIds.isEmpty()) {
				_removeFiles(fileIds);
				_removeFileHashes(fileIds);
			}
		}

		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM messages WHERE accountJid = :accountJid AND chatJid = :chatJid
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
			}
		);

		allMessagesRemovedFromChat(accountJid, chatJid);
	});
}

QFuture<void> MessageDb::removeMessage(const QString &accountJid, const QString &chatJid, const QString &messageId)
{
	return run([this, accountJid, chatJid, messageId]() {
		auto query = createQuery();

		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageId
				LIMIT 1
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":messageId", messageId },
			}
		);

		if (query.first()) {
			// Set the message's content to NULL and the "removed" flag to true.
			execQuery(
				query,
				QStringLiteral(R"(
					UPDATE messages
					SET body = NULL, spoilerHint = NULL, removed = 1
					WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageId
					LIMIT 1
				)"),
				{
					{ u":accountJid", accountJid },
					{ u":chatJid", chatJid },
					{ u":messageId", messageId },
				}
			);

			// Remove reactions corresponding to the removed message.
			execQuery(
				query,
				QStringLiteral(R"(
					DELETE FROM messageReactions
					WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageId = :messageId
				)"),
				{
					{ u":accountJid", accountJid },
					{ u":chatJid", chatJid },
					{ u":messageId", messageId },
				}
			);
		}

		Q_EMIT messageRemoved(_initializeLastMessage(accountJid, chatJid));
	});
}

QFuture<void> MessageDb::updateMessage(const QString &id,
                                       const std::function<void (Message &)> &updateMsg)
{
	return run([this, id, updateMsg]() {
		// load current message item from db
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE id = :messageId OR replaceId = :messageId
				LIMIT 1
			)"),
			{
				{ u":messageId", id },
			}
		);

		auto msgs = _fetchMessagesFromQuery(query);
		_fetchReactions(msgs);

		// update loaded item
		if (!msgs.isEmpty()) {
			const auto &oldMessage = msgs.first();
			Q_ASSERT(oldMessage.deliveryState != DeliveryState::Draft);

			Message newMessage = oldMessage;
			updateMsg(newMessage);
			Q_ASSERT(newMessage.deliveryState != DeliveryState::Draft);

			// Replace the old message's values with the updated ones if the message has changed.
			if (oldMessage != newMessage) {
				Q_EMIT messageUpdated(newMessage);

				const auto &oldReactionSenders = oldMessage.reactionSenders;
				if (const auto &newReactionSenders = newMessage.reactionSenders; oldReactionSenders != newReactionSenders) {
					// Remove old reactions.
					for (auto itr = oldReactionSenders.begin(); itr != oldReactionSenders.end(); ++itr) {
						const auto &senderJid = itr.key();
						const auto &reactionSender = itr.value();

						for (const auto &reaction : reactionSender.reactions) {
							if (!newReactionSenders.value(senderJid).reactions.contains(reaction)) {
								execQuery(
									query,
									QStringLiteral(R"(
										DELETE FROM messageReactions
										WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId AND senderJid = :senderJid AND emoji = :emoji
									)"),
									{
										{ u":accountJid", oldMessage.accountJid },
										{ u":chatJid", oldMessage.chatJid },
										{ u":messageSenderId", oldMessage.senderId },
										{ u":messageId", oldMessage.id },
										{ u":senderJid", senderJid },
										{ u":emoji", reaction.emoji },
									}
								);
							}
						}
					}

					// Add new reactions.
					for (auto itr = newReactionSenders.begin(); itr != newReactionSenders.end(); ++itr) {
						const auto &senderJid = itr.key();
						const auto &reactionSender = itr.value();

						for (const auto &reaction : reactionSender.reactions) {
							if (!oldReactionSenders.value(senderJid).reactions.contains(reaction)) {
								execQuery(
									query,
									QStringLiteral(R"(
										INSERT INTO messageReactions (
											accountJid,
											chatJid,
											messageSenderId,
											messageId,
											senderJid,
											timestamp,
											deliveryState,
											emoji
										)
										VALUES (
											:accountJid,
											:chatJid,
											:messageSenderId,
											:messageId,
											:senderJid,
											:timestamp,
											:deliveryState,
											:emoji
										)
									)"),
									{
										{ u":accountJid", oldMessage.accountJid },
										{ u":chatJid", oldMessage.chatJid },
										{ u":messageSenderId", oldMessage.senderId },
										{ u":messageId", oldMessage.id },
										{ u":senderJid", senderJid },
										{ u":timestamp", reactionSender.latestTimestamp },
										{ u":deliveryState", int(reaction.deliveryState) },
										{ u":emoji", reaction.emoji },
									}
								);
							}
						}
					}
				} else if (auto rec = createUpdateRecord(oldMessage, newMessage); !rec.isEmpty()) {
					auto &driver = sqlDriver();

					// Create an SQL record containing only the differences.
					execQuery(
						query,
						driver.sqlStatement(
							QSqlDriver::UpdateStatement,
							QStringLiteral(DB_TABLE_MESSAGES),
							rec,
							false
						) +
						simpleWhereStatement(&driver, QStringLiteral("id"), oldMessage.id)
					);
				}

				// remove old files
				auto oldFileIds = transform(oldMessage.files, [](const auto &file) {
					return file.id;
				});
				auto newFileIds = transform(newMessage.files, [](const auto &file) {
					return file.id;
				});
				auto removedFileIds = filter(std::move(oldFileIds), [&](auto id) {
					return !newFileIds.contains(id);
				});
				_removeFiles(removedFileIds);
				_removeFileHashes(removedFileIds);

				// add new files, replace changed files
				_setFiles(newMessage.files);
			}
		}
	});
}

QFuture<std::optional<Message>> MessageDb::fetchDraftMessage(const QString &accountJid, const QString &chatJid)
{
	return run([this, accountJid, chatJid]() {
		return _fetchDraftMessage(accountJid, chatJid);
	});
}

QFuture<void> MessageDb::addDraftMessage(const Message &msg)
{
	return run([this, msg]() {
		Q_ASSERT(msg.deliveryState == DeliveryState::Draft);

		const auto addMessage = [this](const Message &message) {
			_addMessage(message);
			Q_EMIT draftMessageAdded(message);
		};

		if (msg.id.isEmpty()) {
			auto copy = msg;
			copy.id = QXmppUtils::generateStanzaUuid();

			addMessage(copy);
		}

		addMessage(msg);
	});
}

QFuture<void> MessageDb::updateDraftMessage(const QString &accountJid, const QString &chatJid, const std::function<void (Message &)> &updateMessage)
{
	return run([this, accountJid, chatJid, updateMessage]() {
		if (const auto oldMessage = _fetchDraftMessage(accountJid, chatJid); oldMessage) {
			Q_ASSERT(oldMessage->deliveryState == DeliveryState::Draft);
			Message newMessage = *oldMessage;
			updateMessage(newMessage);
			Q_ASSERT(newMessage.deliveryState == DeliveryState::Draft);

			// Replace the old message's values with the updated ones if the message has changed.
			if (auto rec = createUpdateRecord(*oldMessage, newMessage); !rec.isEmpty()) {
				auto &driver = sqlDriver();

				// Create an SQL record containing only the differences.
				auto query = createQuery();
				execQuery(
					query,
					driver.sqlStatement(
						QSqlDriver::UpdateStatement,
						QStringLiteral(DB_TABLE_MESSAGES),
						rec,
						false
					) +
					simpleWhereStatement(&driver, QStringLiteral("id"), newMessage.id)
				);

				Q_EMIT draftMessageUpdated(newMessage);
			}
		}
	});
}

QFuture<void> MessageDb::removeDraftMessage(const QString &accountJid, const QString &chatJid)
{
	return run([this, accountJid, chatJid]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				DELETE FROM messages
				WHERE accountJid = :accountJid AND chatJid = :chatJid AND deliveryState = :deliveryState
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":chatJid", chatJid },
				{ u":deliveryState", int(DeliveryState::Draft) },
			}
		);

		Q_EMIT draftMessageRemoved(_initializeLastMessage(accountJid, chatJid));
	});
}

void MessageDb::_addMessage(const Message &message)
{
	// "execQuery()" with "sqlDriver().sqlStatement()" cannot be used here because the binary data
	// of "msg.senderKey()" is not appropriately inserted into the database.
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			INSERT INTO messages (
				accountJid,
				chatJid,
				senderId,
				id,
				originId,
				stanzaId,
				replaceId,
				timestamp,
				body,
				encryption,
				senderKey,
				deliveryState,
				isSpoiler,
				spoilerHint,
				fileGroupId,
				errorText,
				removed
			)
			VALUES (
				:accountJid,
				:chatJid,
				:senderId,
				:id,
				:originId,
				:stanzaId,
				:replaceId,
				:timestamp,
				:body,
				:encryption,
				:senderKey,
				:deliveryState,
				:isSpoiler,
				:spoilerHint,
				:fileGroupId,
				:errorText,
				:removed
			)
		)"),
		{
			{ u":accountJid", message.accountJid },
			{ u":chatJid", message.chatJid },
			{ u":senderId", message.senderId },
			{ u":id", message.id },
			{ u":originId", message.originId },
			{ u":stanzaId", message.stanzaId },
			{ u":replaceId", message.replaceId },
			{ u":timestamp", message.timestamp.toString(Qt::ISODateWithMs) },
			{ u":body", message.body },
			{ u":encryption", message.encryption },
			{ u":senderKey", message.senderKey },
			{ u":deliveryState", int(message.deliveryState) },
			{ u":isSpoiler", message.isSpoiler },
			{ u":spoilerHint", message.spoilerHint },
			{ u":fileGroupId", optionalToVariant(message.fileGroupId) },
			{ u":errorText", message.errorText },
			{ u":removed", message.removed },
		}
	);
}

void MessageDb::_setFiles(const QVector<File> &files)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO files (
					id,
					fileGroupId,
					name,
					description,
					mimeType,
					size,
					lastModified,
					disposition,
					thumbnail,
					localFilePath
				)
				VALUES (
					:id,
					:fileGroupId,
					:name,
					:description,
					:mimeType,
					:size,
					:lastModified,
					:disposition,
					:thumbnail,
					:localFilePath
				)
			)")
		);
		return query;
	}();

	for (const auto &file : files) {
		bindValues(
			query,
			{
				{ u":id", file.id },
				{ u":fileGroupId", file.fileGroupId },
				{ u":name", optionalToVariant(file.name) },
				{ u":description", optionalToVariant(file.description) },
				{ u":mimeType", file.mimeType.name() },
				{ u":size", optionalToVariant(file.size) },
				{ u":lastModified", serialize(file.lastModified) },
				{ u":disposition", int(file.disposition) },
				{ u":thumbnail", file.thumbnail },
				{ u":localFilePath", file.localFilePath },
			}
		);
		execQuery(query);

		_setFileHashes(file.hashes);
		_setHttpSources(file.httpSources);
		_setEncryptedSources(file.encryptedSources);
	}
}

void MessageDb::_setFileHashes(const QVector<FileHash> &fileHashes)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO fileHashes (
					dataId,
					hashType,
					hashValue
				)
				VALUES (
					:dataId,
					:hashType,
					:hashValue
				)
			)")
		);
		return query;
	}();

	for (const auto &hash : fileHashes) {
		bindValues(
			query,
			{
				{ u":dataId", hash.dataId },
				{ u":hashType", int(hash.hashType) },
				{ u":hashValue", hash.hashValue },
			}
		);
		execQuery(query);
	}
}

void MessageDb::_setHttpSources(const QVector<HttpSource> &sources)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO fileHttpSources (
					fileId,
					url
				)
				VALUES (
					:fileId,
					:url
				)
			)")
		);
		return query;
	}();

	for (const auto &source : sources) {
		bindValues(
			query,
			{
				{ u":fileId", source.fileId },
				{ u":url", source.url.toEncoded() },
			}
		);
		execQuery(query);
	}
}

void MessageDb::_setEncryptedSources(const QVector<EncryptedSource> &sources)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(
			query,
			QStringLiteral(R"(
				INSERT OR REPLACE INTO fileEncryptedSources (
					fileId,
					url,
					cipher,
					key,
					iv,
					encryptedDataId
				)
				VALUES (
					:fileId,
					:url,
					:cipher,
					:key,
					:iv,
					:encryptedDataId
				)
			)")
		);
		return query;
	}();

	for (const auto &source : sources) {
		bindValues(
			query,
			{
				{ u":fileId", source.fileId },
				{ u":url", source.url.toEncoded() },
				{ u":cipher", int(source.cipher) },
				{ u":key", source.key },
				{ u":iv", source.iv },
				{ u":encryptedDataId", optionalToVariant(source.encryptedDataId) },
			}
		);
		execQuery(query);

		_setFileHashes(source.encryptedHashes);
	}
}

void MessageDb::_removeFiles(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, QStringLiteral("DELETE FROM files WHERE id = :fileId"));
	for (auto fileId : fileIds) {
		bindValues(query, {{ u":fileId", fileId }});
		execQuery(query);
	}
}

void MessageDb::_removeFileHashes(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, QStringLiteral("DELETE FROM fileHashes WHERE dataId = :fileId"));
	for (auto fileId : fileIds) {
		bindValues(query, {{ u":fileId", fileId }});
		execQuery(query);
	}
}

void MessageDb::_removeHttpSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, QStringLiteral("DELETE FROM fileHttpSources WHERE fileId = :fileId"));
	for (auto fileId : fileIds) {
		bindValues(query, {{ u":fileId", fileId }});
		execQuery(query);
	}
}

void MessageDb::_removeEncryptedSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, QStringLiteral("DELETE FROM fileEncryptedSources WHERE fileId = :fileId"));
	for (auto fileId : fileIds) {
		bindValues(query, {{ u":fileId", fileId }});
		execQuery(query);
	}
}

QVector<File> MessageDb::_fetchFiles(const QString &accountJid)
{
	Q_ASSERT(!accountJid.isEmpty());

	return _fetchFiles(
		QStringLiteral(R"(
			SELECT fileGroupId
			FROM chatMessages
			WHERE accountJid = :accountJid AND fileGroupId IS NOT NULL
		)"),
		{
			{ u":accountJid", accountJid },
		}
	);
}

QVector<File> MessageDb::_fetchFiles(const QString &accountJid, const QString &chatJid)
{
	Q_ASSERT(!accountJid.isEmpty() && !chatJid.isEmpty());

	return _fetchFiles(
		QStringLiteral(R"(
			SELECT fileGroupId
			FROM chatMessages
			WHERE accountJid = :accountJid AND chatJid = :chatJid AND fileGroupId IS NOT NULL
		)"),
		{
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
		}
	);
}

QVector<File> MessageDb::_fetchFiles(const QString &statement, const std::vector<QueryBindValue> &bindValues)
{
	enum { FileGroupId };
	auto query = createQuery();
	execQuery(query, statement, bindValues);

	QVector<File> files;
	reserve(files, query);

	while (query.next()) {
		files.append(_fetchFiles(query.value(FileGroupId).toLongLong()));
	}

	return files;
}

void MessageDb::_extractDownloadedFiles(QVector<File> &files)
{
	files.erase(std::remove_if(files.begin(), files.end(), [](const File &file) {
			return !QFile::exists(file.localFilePath);
		}),
		files.end()
	);
}

QVector<File> MessageDb::_fetchFiles(qint64 fileGroupId)
{
	enum { Id, Name, Description, MimeType, Size, LastModified, Disposition, Thumbnail, LocalFilePath };
	thread_local static auto query = [this]() {
		auto q = createQuery();
		prepareQuery(q,
			QStringLiteral("SELECT id, name, description, mimeType, size, lastModified, disposition, "
			"thumbnail, localFilePath FROM files "
			"WHERE fileGroupId = :fileGroupId"));
		return q;
	}();

	bindValues(query, {{ u":fileGroupId", QVariant(fileGroupId) }});
	execQuery(query);

	QVector<File> files;
	reserve(files, query);
	while (query.next()) {
		auto id = query.value(Id).toLongLong();
		files << File {
			query.value(Id).toLongLong(),
			fileGroupId,
			variantToOptional<QString>(query.value(Name)),
			variantToOptional<QString>(query.value(Description)),
			QMimeDatabase().mimeTypeForName(query.value(MimeType).toString()),
			variantToOptional<long long>(query.value(Size)),
			parseDateTime(query, LastModified),
			query.value(Disposition).value<QXmppFileShare::Disposition>(),
			query.value(LocalFilePath).toString(),
			_fetchFileHashes(id),
			query.value(Thumbnail).toByteArray(),
			_fetchHttpSource(id),
			_fetchEncryptedSource(id),
		};
	}
	return files;
}

QVector<FileHash> MessageDb::_fetchFileHashes(qint64 fileId)
{
	enum { HashType, HashValue };
	thread_local static auto query = [this]() {
		auto q = createQuery();
		prepareQuery(q, QStringLiteral("SELECT hashType, hashValue FROM fileHashes WHERE dataId = :fileId"));
		return q;
	}();

	bindValues(query, {{ u":fileId", fileId }});
	execQuery(query);

	QVector<FileHash> hashes;
	reserve(hashes, query);
	while (query.next()) {
		hashes << FileHash {
			fileId,
			query.value(HashType).value<QXmpp::HashAlgorithm>(),
			query.value(HashValue).toByteArray()
		};
	}
	return hashes;
}

QVector<HttpSource> MessageDb::_fetchHttpSource(qint64 fileId)
{
	enum { Url };
	thread_local static auto query = [this]() {
		auto q = createQuery();
		prepareQuery(q, QStringLiteral("SELECT url FROM fileHttpSources WHERE fileId = :fileId"));
		return q;
	}();

	bindValues(query, {{ u":fileId", fileId }});
	execQuery(query);

	QVector<HttpSource> sources;
	reserve(sources, query);
	while (query.next()) {
		sources << HttpSource {
			fileId,
			QUrl::fromEncoded(query.value(Url).toByteArray())
		};
	}
	return sources;
}

QVector<EncryptedSource> MessageDb::_fetchEncryptedSource(qint64 fileId)
{
	enum { Url, Cipher, Key, Iv, EncryptedDataId };
	thread_local static auto query = [this]() {
		auto q = createQuery();
		prepareQuery(q,
			QStringLiteral("SELECT url, cipher, key, iv, encryptedDataId FROM fileEncryptedSources "
			"WHERE fileId = :fileId"));
		return q;
	}();

	bindValues(query, {{ u":fileId", fileId }});
	execQuery(query);

	auto parseHashes = [this](QSqlQuery &query) -> QVector<FileHash> {
		auto dataId = query.value(EncryptedDataId);
		if (dataId.isNull()) {
			return {};
		}
		return _fetchFileHashes(dataId.toLongLong());
	};

	QVector<EncryptedSource> sources;
	reserve(sources, query);
	while (query.next()) {
		sources << EncryptedSource {
			fileId,
			QUrl::fromEncoded(query.value(Url).toByteArray()),
			query.value(Cipher).value<QXmpp::Cipher>(),
			query.value(Key).toByteArray(),
			query.value(Iv).toByteArray(),
			variantToOptional<qint64>(query.value(EncryptedDataId)),
			parseHashes(query),
		};
	}
	return sources;
}

void MessageDb::_fetchReactions(QVector<Message> &messages)
{
	enum { SenderJid, Emoji, Timestamp, DeliveryState };
	auto query = createQuery();

	for (auto &message : messages) {
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT senderJid, emoji, timestamp, deliveryState
				FROM messageReactions
				WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId
			)"),
			{
				{ u":accountJid", message.accountJid },
				{ u":chatJid", message.chatJid },
				{ u":messageSenderId", message.senderId },
				{ u":messageId", message.id },
			}
		);

		// Iterate over all found emojis.
		while (query.next()) {
			MessageReaction reaction;
			reaction.emoji = query.value(Emoji).toString();
			reaction.deliveryState = query.value(DeliveryState).value<MessageReactionDeliveryState::Enum>();

			auto &reactionSender = message.reactionSenders[query.value(SenderJid).toString()];

			// Use the timestamp of the current emoji as the latest timestamp if the emoji's
			// timestamp is newer than the latest one.
			if (const auto timestamp = query.value(Timestamp).toDateTime(); reactionSender.latestTimestamp < timestamp) {
				reactionSender.latestTimestamp = timestamp;
			}

			reactionSender.reactions.append(reaction);
		}
	}
}

std::optional<Message> MessageDb::_fetchDraftMessage(const QString &accountJid, const QString &chatJid)
{
	auto query = createQuery();
	execQuery(
		query,
		QStringLiteral(R"(
			SELECT *
			FROM draftMessages
			WHERE accountJid = :accountJid AND chatJid = :chatJid
		)"),
		{
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
		}
	);

	const auto messages = _fetchMessagesFromQuery(query);

	if (messages.isEmpty()) {
		return std::nullopt;
	}

	return messages.constFirst();
}

bool MessageDb::_checkMessageExists(const Message &message)
{
	std::vector<QueryBindValue> bindValues = {
		{ u":accountJid", message.accountJid },
		{ u":chatJid", message.chatJid },
	};

	// Check which IDs to check
	QStringList idChecks;
	if (!message.stanzaId.isEmpty()) {
		idChecks << QStringLiteral("stanzaId = :stanzaId");
		bindValues.push_back({ u":stanzaId", message.stanzaId });
	}
	// only check origin IDs if the message was possibly sent by us (since
	// Kaidan uses random suffixes in the resource, we can't check the resource)
	if (message.isOwn() && !message.originId.isEmpty()) {
		idChecks << QStringLiteral("originId = :originId");
		bindValues.push_back({ u":originId", message.originId });
	}
	if (!message.id.isEmpty()) {
		idChecks << QStringLiteral("id = :id");
		bindValues.push_back({ u":id", message.id });
	}

	if (idChecks.isEmpty()) {
		// if we have no checks because of missing IDs, report that the message
		// does not exist
		return false;
	}

	const QString idConditionSql = idChecks.join(u" OR ");

	// By querying DB_TABLE_MESSAGES instead of DB_VIEW_CHAT_MESSAGES and excluding drafts, all sent or received messages are retrieved.
	// That includes locally removed messages.
	// It avoids storing messages that were already locally removed again when received via MAM afterwards.
	const QString querySql =
		QStringLiteral("SELECT COUNT(*) FROM " DB_TABLE_MESSAGES " "
					   "WHERE (accountJid = :accountJid AND chatJid = :chatJid AND (") %
		idConditionSql %
		QStringLiteral(")) AND deliveryState != 4 ") %
		QStringLiteral("ORDER BY timestamp DESC LIMIT " CHECK_MESSAGE_EXISTS_DEPTH_LIMIT);

	auto query = createQuery();
	execQuery(query, querySql, bindValues);

	int count = 0;
	if (query.next()) {
		count = query.value(0).toInt();
	}
	return count > 0;
}

QFuture<QVector<Message>> MessageDb::fetchPendingMessages(const QString &accountJid)
{
	return run([this, accountJid]() {
		auto query = createQuery();
		execQuery(
			query,
			QStringLiteral(R"(
				SELECT *
				FROM chatMessages
				WHERE accountJid = :accountJid AND deliveryState = :deliveryState
				ORDER BY timestamp ASC
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":deliveryState", int(Enums::DeliveryState::Pending) },
			}
		);

		return _fetchMessagesFromQuery(query);
	});
}

QFuture<QMap<QString, QMap<QString, MessageReactionSender>>> MessageDb::fetchPendingReactions(const QString &accountJid)
{
	return run([this, accountJid]() {
		enum { ChatJid, MessageSenderId, MessageId };
		auto pendingReactionQuery = createQuery();
		execQuery(
			pendingReactionQuery,
			QStringLiteral(R"(
				SELECT DISTINCT chatJid, messageSenderId, messageId
				FROM messageReactions
				WHERE
					accountJid = :accountJid AND senderJid = :accountJid AND
					(deliveryState = :deliveryState1 OR deliveryState = :deliveryState2 OR deliveryState = :deliveryState3)
			)"),
			{
				{ u":accountJid", accountJid },
				{ u":deliveryState1", int(MessageReactionDeliveryState::PendingAddition) },
				{ u":deliveryState2", int(MessageReactionDeliveryState::PendingRemovalAfterSent) },
				{ u":deliveryState3", int(MessageReactionDeliveryState::PendingRemovalAfterDelivered) },
			}
		);

		// IDs of chats mapped to IDs of messages mapped to MessageReactionSender
		QMap<QString, QMap<QString, MessageReactionSender>> reactions;

		// Iterate over all IDs of messages with pending reactions.
		while (pendingReactionQuery.next()) {
			const auto chatJid = pendingReactionQuery.value(ChatJid).toString();
			const auto messageSenderId = pendingReactionQuery.value(MessageSenderId).toString();
			const auto messageId = pendingReactionQuery.value(MessageId).toString();

			enum { Emoji, Timestamp, DeliveryState };
			auto reactionQuery = createQuery();
			execQuery(
				reactionQuery,
				QStringLiteral(R"(
					SELECT emoji, timestamp, deliveryState
					FROM messageReactions
					WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId AND senderJid = :accountJid
				)"),
				{
					{ u":accountJid", accountJid },
					{ u":chatJid", chatJid },
					{ u":messageSenderId", messageSenderId },
					{ u":messageId", messageId },
				}
			);

			// Iterate over all reactions of messages with pending reactions.
			while (reactionQuery.next()) {
				auto &reactionSender = reactions[chatJid][messageId];

				// Use the timestamp of the current emoji as the latest timestamp if the emoji's
				// timestamp is newer than the latest one.
				if (const auto timestamp = reactionQuery.value(Timestamp).toDateTime(); reactionSender.latestTimestamp < timestamp) {
					reactionSender.latestTimestamp = timestamp;
				}

				MessageReaction reaction;
				reaction.emoji = reactionQuery.value(Emoji).toString();
				reaction.deliveryState = MessageReactionDeliveryState::Enum(reactionQuery.value(DeliveryState).toInt());

				reactionSender.reactions.append(reaction);
			}
		}

		return reactions;
	});
}

Message MessageDb::_initializeLastMessage(const QString &accountJid, const QString &chatJid)
{
	auto message = _fetchLastMessage(accountJid, chatJid);

	// The retrieved last message can be a default-constructed message if the removed message was
	// the last one of the corresponding chat.
	// In that case, the account and chat JIDs are set in order to relate the message to its chat.
	if (message.accountJid.isEmpty()) {
		message.accountJid = accountJid;
		message.chatJid = chatJid;
	}

	return message;
}
