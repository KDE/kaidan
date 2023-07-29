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
#include <QSqlQuery>
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
#include "SqlUtils.h"

using namespace SqlUtils;

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
	int idxFrom = rec.indexOf("sender");
	int idxTo = rec.indexOf("recipient");
	int idxStamp = rec.indexOf("timestamp");
	int idxId = rec.indexOf("id");
	int idxEncryption = rec.indexOf("encryption");
	int idxSenderKey = rec.indexOf("senderKey");
	int idxBody = rec.indexOf("body");
	int idxDeliveryState = rec.indexOf("deliveryState");
	int idxSpoilerHint = rec.indexOf("spoilerHint");
	int idxIsSpoiler = rec.indexOf("isSpoiler");
	int idxErrorText = rec.indexOf("errorText");
	int idxReplaceId = rec.indexOf("replaceId");
	int idxOriginId = rec.indexOf("originId");
	int idxStanza = rec.indexOf("stanzaId");
	int idxFileGroupId = rec.indexOf("fileGroupId");
	int idxRemoved = rec.indexOf("removed");

	reserve(messages, query);
	while (query.next()) {
		Message msg;
		msg.from = query.value(idxFrom).toString();
		msg.to = query.value(idxTo).toString();
		msg.stamp = QDateTime::fromString(
			query.value(idxStamp).toString(),
			Qt::ISODate
		);
		msg.id = query.value(idxId).toString();
		msg.encryption = Encryption::Enum(query.value(idxEncryption).toInt());
		msg.senderKey = query.value(idxSenderKey).toByteArray();
		msg.body = query.value(idxBody).toString();
		msg.deliveryState = static_cast<Enums::DeliveryState>(query.value(idxDeliveryState).toInt());
		msg.spoilerHint = query.value(idxSpoilerHint).toString();
		msg.errorText = query.value(idxErrorText).toString();
		msg.isSpoiler = query.value(idxIsSpoiler).toBool();
		msg.replaceId = query.value(idxReplaceId).toString();
		msg.originId = query.value(idxOriginId).toString();
		msg.stanzaId = query.value(idxStanza).toString();
		msg.fileGroupId = variantToOptional<qint64>(query.value(idxFileGroupId));
		// this is useful with resending pending messages
		msg.receiptRequested = true;
		msg.removed = query.value(idxRemoved).toBool();

		// fetch referenced files
		if (msg.fileGroupId) {
			msg.files = _fetchFiles(*msg.fileGroupId);
		}

		messages << std::move(msg);
	}
	return messages;
}

QSqlRecord MessageDb::createUpdateRecord(const Message &oldMsg, const Message &newMsg)
{
	QSqlRecord rec;

	if (oldMsg.from != newMsg.from)
		rec.append(createSqlField("sender", newMsg.from));
	if (oldMsg.to != newMsg.to)
		rec.append(createSqlField("recipient", newMsg.to));
	if (oldMsg.stamp != newMsg.stamp)
		rec.append(createSqlField(
		        "timestamp",
		        newMsg.stamp.toString(Qt::ISODateWithMs)
		));
	if (oldMsg.id != newMsg.id) {
		rec.append(createSqlField("id", newMsg.id));
	}
	if (oldMsg.encryption != newMsg.encryption)
		rec.append(createSqlField("encryption", newMsg.encryption));
	if (oldMsg.senderKey != newMsg.senderKey)
		rec.append(createSqlField("senderKey", newMsg.senderKey));
	if (oldMsg.body != newMsg.body)
		rec.append(createSqlField("body", newMsg.body));
	if (oldMsg.deliveryState != newMsg.deliveryState)
		rec.append(createSqlField("deliveryState", int(newMsg.deliveryState)));
	if (oldMsg.errorText != newMsg.errorText)
		rec.append(createSqlField("errorText", newMsg.errorText));
	if (oldMsg.spoilerHint != newMsg.spoilerHint)
		rec.append(createSqlField("spoilerHint", newMsg.spoilerHint));
	if (oldMsg.isSpoiler != newMsg.isSpoiler)
		rec.append(createSqlField("isSpoiler", newMsg.isSpoiler));
	if (oldMsg.replaceId != newMsg.replaceId)
		rec.append(createSqlField("replaceId", newMsg.replaceId));
	if (oldMsg.originId != newMsg.originId)
		rec.append(createSqlField("originId", newMsg.originId));
	if (oldMsg.stanzaId != newMsg.stanzaId)
		rec.append(createSqlField("stanzaId", newMsg.stanzaId));
	if (oldMsg.fileGroupId != newMsg.fileGroupId) {
		rec.append(createSqlField("fileGroupId", optionalToVariant(newMsg.fileGroupId)));
	}
	if (oldMsg.removed != newMsg.removed) {
		rec.append(createSqlField("removed", newMsg.removed));
	}

	return rec;
}

QFuture<QVector<Message>> MessageDb::fetchMessages(const QString &accountJid, const QString &chatJid, int index)
{
	return run([this, accountJid, chatJid, index]() {
		auto query = createQuery();
		prepareQuery(
			query,
			"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE (sender = :accountJid AND recipient = :chatJid) OR "
				  "(sender = :chatJid AND recipient = :accountJid) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, :limit"
		);
		bindValues(query, {
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
			{ u":index", index },
			{ u":limit", DB_QUERY_LIMIT_MESSAGES },
		});
		execQuery(query);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		emit messagesFetched(messages);
		return messages;
	});
}

QFuture<QVector<File>> MessageDb::fetchFiles(const QString &accountJid, const QString &chatJid)
{
	return _fetchFiles(accountJid, chatJid, false);
}

QFuture<QVector<File>> MessageDb::fetchDownloadedFiles(const QString &accountJid, const QString &chatJid)
{
	return _fetchFiles(accountJid, chatJid, true);
}

QFuture<QVector<Message> > MessageDb::fetchMessagesUntilFirstContactMessage(const QString &accountJid, const QString &chatJid, int index)
{
	return run([this, accountJid, chatJid, index]() {
		auto query = createQuery();
		prepareQuery(
			query,
			R"(
				SELECT * FROM chatMessages
				WHERE
					(sender = :accountJid AND recipient = :chatJid) OR
					(sender = :chatJid AND recipient = :accountJid)
				ORDER BY timestamp DESC
				LIMIT
					:index,
					:limit + (
						SELECT COUNT() FROM chatMessages
						WHERE
							timestamp >= (
								SELECT timestamp FROM chatMessages
								WHERE sender = :chatJid AND recipient = :accountJid)
							AND (
								(sender = :accountJid AND recipient = :chatJid) OR
								(sender = :chatJid AND recipient = :accountJid)
							)
					)
			)"
		);
		bindValues(query, {
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
			{ u":index", index },
			{ u":limit", DB_QUERY_LIMIT_MESSAGES },
		});
		execQuery(query);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		emit messagesFetched(messages);
		return messages;
	});
}

QFuture<QVector<Message>> MessageDb::fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId)
{
	return run([this, accountJid, chatJid, index, limitingId]() {
		auto query = createQuery();
		prepareQuery(
			query,
			"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE (sender = :accountJid AND recipient = :chatJid) OR "
			"(sender = :chatJid AND recipient = :accountJid) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, ("
			"SELECT COUNT() FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE timestamp >= "
			"(SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE sender = :chatJid AND recipient = :accountJid AND id = :id) AND "
			"((sender = :accountJid AND recipient = :chatJid) OR "
			"(sender = :chatJid AND recipient = :accountJid)) "
			") + :limit"
		);
		bindValues(query, {
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
			{ u":index", index },
			{ u":limit", DB_QUERY_LIMIT_MESSAGES },
			{ u":id", limitingId },
		});
		execQuery(query);

		auto messages = _fetchMessagesFromQuery(query);
		_fetchReactions(messages);

		emit messagesFetched(messages);
		return messages;
	});
}

QFuture<MessageDb::MessageResult> MessageDb::fetchMessagesUntilQueryString(const QString &accountJid, const QString &chatJid, int index, const QString &queryString)
{
	return run([this, accountJid, chatJid, index, queryString]() -> MessageResult {
		auto query = createQuery();

		prepareQuery(
			query,
			"SELECT COUNT() FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE timestamp >= "
			"(SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE ((sender = :chatJid AND recipient = :accountJid) OR (sender = :accountJid AND recipient = :chatJid)) AND "
			"body LIKE :queryString AND "
			"timestamp <= "
			"(SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE ((sender = :chatJid AND recipient = :accountJid) OR (sender = :accountJid AND recipient = :chatJid)) "
			"ORDER BY timestamp DESC LIMIT :index, 1) "
			"ORDER BY timestamp DESC LIMIT 1) AND "
			"((sender = :accountJid AND recipient = :chatJid) OR "
			"(sender = :chatJid AND recipient = :accountJid))"
		);
		bindValues(query, {
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
			{ u":index", index },
			// '%' is intended here as a placeholder inside the query for SQL statement "LIKE".
			{ u":queryString", "%" + queryString + "%" },
		});
		execQuery(query);

		query.first();
		const auto queryStringMessageIndex = query.value(0).toInt();
		const auto messagesUntilQueryStringCount = queryStringMessageIndex - index;

		// Skip further processing if no message with queryString could be found.
		if (messagesUntilQueryStringCount <= 0) {
			return {};
		}

		prepareQuery(
			query,
			"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE (sender = :accountJid AND recipient = :chatJid) OR "
			"(sender = :chatJid AND recipient = :accountJid) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, :limit"
		);
		bindValues(query, {
			{ u":accountJid", accountJid },
			{ u":chatJid", chatJid },
			{ u":index", index },
			{ u":limit", messagesUntilQueryStringCount + DB_QUERY_LIMIT_MESSAGES },
		});
		execQuery(query);

		MessageResult result {
			_fetchMessagesFromQuery(query),
			// Database index starts at 1, but message model index starts at 0.
			queryStringMessageIndex - 1
		};

		_fetchReactions(result.messages);

		emit messagesFetched(result.messages);

		return result;
	});
}

Message MessageDb::_fetchLastMessage(const QString &user1, const QString &user2)
{
	auto query = createQuery();
	execQuery(
		query,
		"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " "
		"WHERE (sender = :user1 AND recipient = :user2) OR "
		      "(sender = :user2 AND recipient = :user1) "
		"ORDER BY timestamp DESC "
		"LIMIT 1",
		{ { u":user1", user1 }, { u":user2", user2 } }
	);

	auto messages = _fetchMessagesFromQuery(query);

	if (!messages.isEmpty())
		return messages.first();
	return {};
}

QFuture<QDateTime> MessageDb::fetchLastMessageStamp()
{
	return run([this]() {
		auto query = createQuery();
		execQuery(query, "SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " ORDER BY timestamp DESC LIMIT 1");

		QDateTime stamp;
		while (query.next()) {
			stamp = QDateTime::fromString(
				query.value(query.record().indexOf("timestamp")).toString(),
				Qt::ISODate
			);
		}

		emit lastMessageStampFetched(stamp);
		return stamp;
	});
}

QFuture<QDateTime> MessageDb::messageTimestamp(const QString &senderJid, const QString &recipientJid, const QString &messageId)
{
	return run([=, this]() {
		auto query = createQuery();
		execQuery(
			query,
			"SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " DESC WHERE sender = ? AND recipient = ? AND id = ? LIMIT 1",
			{ senderJid, recipientJid, messageId }
		);

		if (query.first()) {
			return QDateTime::fromString(
				query.value(0).toString(),
				Qt::ISODateWithMs
			);
		}

		return QDateTime();
	});
}

QFuture<QString> MessageDb::firstContactMessageId(const QString &accountJid, const QString &chatJid, int index)
{
	return run([=, this]() {
		auto query = createQuery();
		execQuery(
			query,
			"SELECT id FROM " DB_VIEW_CHAT_MESSAGES " WHERE sender = ? AND recipient = ? ORDER BY timestamp DESC LIMIT ?, 1",
			{ chatJid, accountJid, index }
		);

		if (query.first()) {
			return query.value(0).toString();
		}

		return QString();
	});
}

QFuture<int> MessageDb::messageCount(const QString &senderJid, const QString &recipientJid, const QString &messageIdBegin, const QString &messageIdEnd)
{
	return run([=, this]() {
		auto query = createQuery();
		execQuery(
			query,
			"SELECT COUNT(*) FROM " DB_VIEW_CHAT_MESSAGES " DESC WHERE sender = ? AND recipient = ? AND "
			"datetime(timestamp) BETWEEN "
			"datetime((SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " DESC WHERE "
			"sender = ? AND recipient = ? AND id = ? LIMIT 1)) AND "
			"datetime((SELECT timestamp FROM " DB_VIEW_CHAT_MESSAGES " DESC WHERE "
			"sender = ? AND recipient = ? AND id = ? LIMIT 1))",
			{ senderJid, recipientJid, senderJid, recipientJid, messageIdBegin, senderJid, recipientJid, messageIdEnd }
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
				if (msg.isOwn) {
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
		emit messageAdded(msg, origin);

		// "execQuery()" with "sqlDriver().sqlStatement()" cannot be used here
		// because the binary data of "msg.senderKey()" is not appropriately
		// inserted into the database.

		auto query = createQuery();
		prepareQuery(
			query,
			"INSERT INTO messages (sender, recipient, timestamp, body, id, encryption, "
			"senderKey, deliveryState, isSpoiler, spoilerHint, errorText, replaceId, "
			"originId, stanzaId, fileGroupId, removed) "
			"VALUES (:sender, :recipient, :timestamp, :body, :id, :encryption, :senderKey, "
			":deliveryState, :isSpoiler, :spoilerHint, :errorText, :replaceId, "
			":originId, :stanzaId, :fileGroupId, :removed)"
		);

		bindValues(query, {
			{ u":sender", msg.from },
			{ u":recipient", msg.to },
			{ u":timestamp", msg.stamp.toString(Qt::ISODateWithMs) },
			{ u":body", msg.body },
			{ u":id", msg.id.isEmpty() ? " " : msg.id },
			{ u":encryption", msg.encryption },
			{ u":senderKey", msg.senderKey },
			{ u":deliveryState", int(msg.deliveryState) },
			{ u":isSpoiler", msg.isSpoiler },
			{ u":spoilerHint", msg.spoilerHint },
			{ u":errorText", msg.errorText },
			{ u":replaceId", msg.replaceId },
			{ u":originId", msg.originId },
			{ u":stanzaId", msg.stanzaId },
			{ u":fileGroupId", optionalToVariant(msg.fileGroupId) },
			{ u":removed", msg.removed }
		});
		execQuery(query);

		_setFiles(msg.files);
	});
}

QFuture<void> MessageDb::removeMessages(const QString &, const QString &)
{
	return run([this]() {
		auto query = createQuery();

		// remove files
		{
			execQuery(query, "SELECT fileGroupId FROM messages WHERE fileGroupId IS NOT NULL");

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

		execQuery(query, "DELETE FROM " DB_TABLE_MESSAGES);
	});
}

QFuture<void> MessageDb::removeMessage(const QString &senderJid, const QString &recipientJid,
									   const QString &messageId)
{
	return run([this, senderJid, recipientJid, messageId]() {
		auto query = createQuery();

		execQuery(
			query,
			"SELECT * FROM chatMessages "
			"WHERE id = :messageId "
			"AND ((sender = :sender AND recipient = :recipient) OR "
			"(recipient = :sender AND sender = :recipient)) "
			"LIMIT 1",
			{{ u":messageId", messageId },
			 { u":recipient", recipientJid },
			 { u":sender", senderJid }}
		);

		auto messages = _fetchMessagesFromQuery(query);

		if (!messages.isEmpty()) {
			// Set the message's content to NULL and the "removed" flag to true.
			execQuery(
				query,
				"UPDATE messages "
				"SET body = NULL, spoilerHint = NULL, removed = 1 "
				"WHERE id = :messageId "
				"AND ((sender = :sender AND recipient = :recipient) OR "
				"(recipient = :sender AND sender = :recipient)) ",
				{{ u":messageId", messageId },
				 { u":recipient", recipientJid },
				 { u":sender", senderJid }}
			);

			// Remove reactions corresponding to the removed message.
			execQuery(
				query,
				"DELETE FROM messageReactions "
				"WHERE messageId = :messageId "
				"AND ((messageSender = :sender AND messageRecipient = :recipient) "
				"OR (messageRecipient = :sender AND messageSender = :recipient)) "
				"LIMIT 1",
				{{ u":messageId", messageId },
				 { u":recipient", recipientJid },
				 { u":sender", senderJid }}
			);
		}

		std::shared_ptr<Message> message {
			std::make_shared<Message>(_fetchLastMessage(senderJid, recipientJid))
		};

		// The retrieved last message can be a default-constructed message if the removed
		// message was the last one of the corresponding chat. In that case, the sender and
		// recipient JIDs are set in order to relate the message to its chat.
		if (message->from.isEmpty()) {
			message->from = senderJid;
			message->to = recipientJid;
		}

		emit messageRemoved(message);
	});
}

QFuture<void> MessageDb::updateMessage(const QString &id,
                                       const std::function<void (Message &)> &updateMsg)
{
	return run([this, id, updateMsg]() {
		emit messageUpdated(id, updateMsg);

		// load current message item from db
		auto query = createQuery();
		execQuery(
			query,
			"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " WHERE id = ? LIMIT 1",
			{ id }
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
									"DELETE FROM " DB_TABLE_MESSAGE_REACTIONS " "
									"WHERE messageSender = :messageSender AND messageRecipient = :messageRecipient AND messageId = :messageId AND senderJid = :senderJid AND emoji = :emoji",
									{ { u":messageSender", oldMessage.from },
									  { u":messageRecipient", oldMessage.to },
									  { u":messageId", oldMessage.id },
									  { u":senderJid", senderJid },
									  { u":emoji", reaction.emoji } }
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
									"INSERT INTO " DB_TABLE_MESSAGE_REACTIONS " "
									"(messageSender, messageRecipient, messageId, senderJid, timestamp, deliveryState, emoji) "
									"VALUES (:messageSender, :messageRecipient, :messageId, :senderJid, :timestamp, :deliveryState, :emoji)",
									{ { u":messageSender", oldMessage.from },
									  { u":messageRecipient", oldMessage.to },
									  { u":messageId", oldMessage.id },
									  { u":senderJid", senderJid },
									  { u":timestamp", reactionSender.latestTimestamp },
									  { u":deliveryState", int(reaction.deliveryState) },
									  { u":emoji", reaction.emoji } }
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
							DB_TABLE_MESSAGES,
							rec,
							false
						) +
						simpleWhereStatement(&driver, "id", id)
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

QFuture<Message> MessageDb::addDraftMessage(const Message &msg)
{
	Q_ASSERT(msg.deliveryState == DeliveryState::Draft);

	auto copy = msg;

	if (copy.id.isEmpty()) {
		copy.id = QXmppUtils::generateStanzaUuid();
	}

	return run([this, msg = std::move(copy)]() {
		// "execQuery()" with "sqlDriver().sqlStatement()" cannot be used here
		// because the binary data of "msg.senderKey()" is not appropriately
		// inserted into the database.

		auto query = createQuery();
		prepareQuery(
			query,
			"INSERT INTO " DB_TABLE_MESSAGES " (sender, recipient, timestamp, body, id, encryption, "
			"senderKey, deliveryState, isSpoiler, spoilerHint, errorText, replaceId, "
			"originId, stanzaId, fileGroupId, removed) "
			"VALUES (:sender, :recipient, :timestamp, :body, :id, :encryption, :senderKey, "
			":deliveryState, :isSpoiler, :spoilerHint, :errorText, :replaceId, "
			":originId, :stanzaId, :fileGroupId, :removed)"
		);

		bindValues(query, {
			{ u":sender", msg.from },
			{ u":recipient", msg.to },
			{ u":timestamp", msg.stamp.toString(Qt::ISODateWithMs) },
			{ u":body", msg.body },
			{ u":id", msg.id },
			{ u":encryption", msg.encryption },
			{ u":senderKey", msg.senderKey },
			{ u":deliveryState", int(msg.deliveryState) },
			{ u":isSpoiler", msg.isSpoiler },
			{ u":spoilerHint", msg.spoilerHint },
			{ u":errorText", msg.errorText },
			{ u":replaceId", msg.replaceId },
			{ u":originId", msg.originId },
			{ u":stanzaId", msg.stanzaId },
			{ u":fileGroupId", optionalToVariant(msg.fileGroupId) },
			{ u":removed", msg.removed }
		});
		execQuery(query);

		emit draftMessageAdded(msg);

		return msg;
	});
}

QFuture<Message> MessageDb::updateDraftMessage(const Message &msg)
{
	Q_ASSERT(msg.deliveryState == DeliveryState::Draft);

	return run([this, msg]() {
		// load current message item from db
		auto query = createQuery();
		execQuery(
			query,
			"SELECT * FROM " DB_VIEW_DRAFT_MESSAGES " WHERE id = ? LIMIT 1",
			{ msg.id }
		);

		auto msgs = _fetchMessagesFromQuery(query);

		// update loaded item
		if (msgs.count() == 1) {
			const auto &oldMessage = msgs.constFirst();

			// Replace the old message's values with the updated ones if the message has changed.
			if (oldMessage != msg) {
				if (auto rec = createUpdateRecord(oldMessage, msg); rec.count()) {
					auto &driver = sqlDriver();

					// Create an SQL record with only the differences.
					execQuery(
						query,
						driver.sqlStatement(
							QSqlDriver::UpdateStatement,
							DB_TABLE_MESSAGES,
							rec,
							false
						) +
						simpleWhereStatement(&driver, "id", msg.id)
					);

					emit draftMessageUpdated(msg);

					return msg;
				}
			}
		}

		return Message();
	});
}

QFuture<QString> MessageDb::removeDraftMessage(const QString &id)
{
	return run([this, id]() {
		auto query = createQuery();
		prepareQuery(query, "DELETE FROM " DB_TABLE_MESSAGES " WHERE id = ? AND deliveryState = ?");
		bindValues(query, { id, int(DeliveryState::Draft) });
		execQuery(query);

		emit draftMessageRemoved(id);

		return id;
	});
}

QFuture<Message> MessageDb::fetchDraftMessage(const QString &id)
{
	return run([this, id]() {
		auto query = createQuery();
		execQuery(
			query,
			"SELECT * FROM " DB_VIEW_DRAFT_MESSAGES " WHERE id = ? LIMIT 1",
			{ id }
		);

		auto msgs = _fetchMessagesFromQuery(query);

		if (msgs.count() == 1) {
			emit draftMessageFetched(msgs.constFirst());

			return msgs.constFirst();
		}

		return Message();
	});
}

void MessageDb::_setFiles(const QVector<File> &files)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(query, "INSERT OR REPLACE INTO files VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");
		return query;
	}();

	for (const auto &file : files) {
		bindValues(query, {
			file.id,
			file.fileGroupId,
			optionalToVariant(file.name),
			optionalToVariant(file.description),
			file.mimeType.name(),
			optionalToVariant(file.size),
			serialize(file.lastModified),
			int(file.disposition),
			file.thumbnail,
			file.localFilePath });
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
		prepareQuery(query, "INSERT OR REPLACE INTO fileHashes VALUES (?, ?, ?)");
		return query;
	}();

	for (const auto &hash : fileHashes) {
		bindValues(query, {
			hash.dataId,
			int(hash.hashType),
			hash.hashValue });
		execQuery(query);
	}
}

void MessageDb::_setHttpSources(const QVector<HttpSource> &sources)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(query, "INSERT OR REPLACE INTO fileHttpSources VALUES (?, ?)");
		return query;
	}();

	for (const auto &source : sources) {
		bindValues(query, { source.fileId, source.url.toEncoded() });
		execQuery(query);
	}
}

void MessageDb::_setEncryptedSources(const QVector<EncryptedSource> &sources)
{
	thread_local static auto query = [this]() {
		auto query = createQuery();
		prepareQuery(query, "INSERT OR REPLACE INTO fileEncryptedSources VALUES (?, ?, ?, ?, ?, ?)");
		return query;
	}();

	for (const auto &source : sources) {
		bindValues(query, { source.fileId, source.url.toEncoded(), int(source.cipher), source.key, source.iv, optionalToVariant(source.encryptedDataId) });
		execQuery(query);

		_setFileHashes(source.encryptedHashes);
	}
}

void MessageDb::_removeFiles(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM files WHERE id = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

void MessageDb::_removeFileHashes(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM fileHashes WHERE dataId = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

void MessageDb::_removeHttpSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM fileHttpSources WHERE fileId = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

void MessageDb::_removeEncryptedSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM fileEncryptedSources WHERE fileId = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

QVector<File> MessageDb::_fetchFiles(qint64 fileGroupId)
{
	enum { Id, Name, Description, MimeType, Size, LastModified, Disposition, Thumbnail, LocalFilePath };
	thread_local static auto query = [this]() {
		auto q = createQuery();
		prepareQuery(q,
			"SELECT id, name, description, mimeType, size, lastModified, disposition, "
			"thumbnail, localFilePath FROM files "
			"WHERE fileGroupId = :fileGroupId");
		return q;
	}();

	bindValues(query, {QueryBindValue {u":fileGroupId", QVariant(fileGroupId)}});
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
			QXmppFileShare::Disposition(query.value(Disposition).toInt()),
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
		prepareQuery(q, "SELECT hashType, hashValue FROM fileHashes WHERE dataId = ?");
		return q;
	}();

	bindValues(query, { QVariant(fileId) });
	execQuery(query);

	QVector<FileHash> hashes;
	reserve(hashes, query);
	while (query.next()) {
		hashes << FileHash {
			fileId,
			QXmpp::HashAlgorithm(query.value(HashType).toInt()),
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
		prepareQuery(q, "SELECT url FROM fileHttpSources WHERE fileId = ?");
		return q;
	}();

	bindValues(query, { QVariant(fileId) });
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
			"SELECT url, cipher, key, iv, encryptedDataId FROM fileEncryptedSources "
			"WHERE fileId = ?");
		return q;
	}();

	bindValues(query, { QVariant(fileId) });
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
			QXmpp::Cipher(query.value(Cipher).toInt()),
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
	enum { SenderJid, Timestamp, DeliveryState, Emoji };
	auto query = createQuery();

	for (auto &message : messages) {
		execQuery(
			query,
			"SELECT senderJid, timestamp, deliveryState, emoji FROM messageReactions "
			"WHERE messageSender = :messageSender AND messageRecipient = :messageRecipient AND messageId = :messageId",
			{ { u":messageSender", message.from }, { u":messageRecipient", message.to }, { u":messageId", message.id } }
		);

		// Iterate over all found emojis.
		while (query.next()) {
			auto &reactionSender = message.reactionSenders[query.value(SenderJid).toString()];

			// Use the timestamp of the current emoji as the latest timestamp if the emoji's
			// timestamp is newer than the latest one.
			if (const auto timestamp = query.value(Timestamp).toDateTime(); reactionSender.latestTimestamp < timestamp) {
				reactionSender.latestTimestamp = timestamp;
			}

			MessageReaction reaction;
			reaction.deliveryState = MessageReactionDeliveryState::Enum(query.value(DeliveryState).toInt());
			reaction.emoji = query.value(Emoji).toString();

			reactionSender.reactions.append(reaction);
		}
	}
}

QFuture<QVector<File> > MessageDb::_fetchFiles(const QString &accountJid, const QString &chatJid, bool checkExists)
{
	return run([this, accountJid, chatJid, checkExists]() {
		const QString vice =
			chatJid.isEmpty()
				? QStringLiteral("sender = :accountJid")
				: QStringLiteral("(sender = :accountJid AND recipient = :chatJid)");
		const QString versa =
			chatJid.isEmpty()
				? QStringLiteral("recipient = :accountJid")
				: QStringLiteral("(sender = :chatJid AND recipient = :accountJid)");
		auto query = createQuery();
		prepareQuery(query,
			"SELECT fileGroupId FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE (" % vice % " OR " % versa % ") AND "
			"fileGroupId IS NOT NULL");
		bindValues(query,
			{
				{u":accountJid", accountJid},
				{u":chatJid", chatJid},
			});
		execQuery(query);

		QVector<File> files;
		reserve(files, query);
		while (query.next()) {
			auto fetched = _fetchFiles(query.value(0).toLongLong());

			if (checkExists) {
				fetched.erase(std::remove_if(fetched.begin(),
						      fetched.end(),
						      [](const File &file) {
							      return !QFile::exists(file.localFilePath);
						      }),
					fetched.end());
			}

			files.append(fetched);
		}
		return files;
	});
}

bool MessageDb::_checkMessageExists(const Message &message)
{
	std::vector<QueryBindValue> bindValues = {
		{ u":to", message.to },
		{ u":from", message.from },
	};

	// Check which IDs to check
	QStringList idChecks;
	if (!message.stanzaId.isEmpty()) {
		idChecks << QStringLiteral("stanzaId = :stanzaId");
		bindValues.push_back({ u":stanzaId", message.stanzaId });
	}
	// only check origin IDs if the message was possibly sent by us (since
	// Kaidan uses random suffixes in the resource, we can't check the resource)
	if (message.isOwn && !message.originId.isEmpty()) {
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
		               "WHERE (sender = :from AND recipient = :to AND (") %
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

QFuture<QVector<Message>> MessageDb::fetchPendingMessages(const QString &userJid)
{
	return run([this, userJid]() {
		auto query = createQuery();
		execQuery(
			query,
			"SELECT * FROM " DB_VIEW_CHAT_MESSAGES " "
			"WHERE (sender = :user AND deliveryState = :deliveryState) "
			"ORDER BY timestamp ASC",
			{
				{ u":user", userJid },
				{ u":deliveryState", int(Enums::DeliveryState::Pending) },
			}
		);

		auto messages = _fetchMessagesFromQuery(query);

		emit pendingMessagesFetched(messages);
		return messages;
	});
}

QFuture<QMap<QString, QMap<QString, MessageReactionSender>>> MessageDb::fetchPendingReactions(const QString &accountJid)
{
	return run([this, accountJid]() {
		enum { MessageSender, MessageId };
		auto pendingReactionQuery = createQuery();

		execQuery(
			pendingReactionQuery,
			"SELECT DISTINCT messageSender, messageId FROM messageReactions "
			"WHERE senderJid = :senderJid AND "
			"deliveryState = :deliveryState1 OR deliveryState = :deliveryState2 OR deliveryState = :deliveryState3",
			{
				{ u":senderJid", accountJid },
				{ u":deliveryState1", int(MessageReactionDeliveryState::PendingAddition) },
				{ u":deliveryState2", int(MessageReactionDeliveryState::PendingRemovalAfterSent) },
				{ u":deliveryState3", int(MessageReactionDeliveryState::PendingRemovalAfterDelivered) },
			}
		);

		// messageSender mapped to messageId mapped to MessageReactionSender
		QMap<QString, QMap<QString, MessageReactionSender>> reactions;

		// Iterate over all IDs of messages with pending reactions.
		while (pendingReactionQuery.next()) {
			enum { Timestamp, DeliveryState, Emoji };
			const auto messageSender = pendingReactionQuery.value(MessageSender).toString();
			const auto messageId = pendingReactionQuery.value(MessageId).toString();
			auto reactionQuery = createQuery();

			execQuery(
				reactionQuery,
				"SELECT timestamp, deliveryState, emoji FROM messageReactions "
				"WHERE messageSender = :messageSender AND messageId = :messageId AND senderJid = :senderJid",
				{
					{ u":messageSender", messageSender },
					{ u":messageId", messageId },
					{ u":senderJid", accountJid },
				}
			);

			// Iterate over all reactions of messages with pending reactions.
			while (reactionQuery.next()) {
				auto &reactionSender = reactions[messageSender][messageId];

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
