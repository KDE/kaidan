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
	int idxBody = rec.indexOf("message");
	int idxDeliveryState = rec.indexOf("deliveryState");
	int idxIsMarkable = rec.indexOf("isMarkable");
	int idxIsEdited = rec.indexOf("isEdited");
	int idxSpoilerHint = rec.indexOf("spoilerHint");
	int idxIsSpoiler = rec.indexOf("isSpoiler");
	int idxErrorText = rec.indexOf("errorText");
	int idxReplaceId = rec.indexOf("replaceId");
	int idxOriginId = rec.indexOf("originId");
	int idxStanza = rec.indexOf("stanzaId");
	int idxFileGroupId = rec.indexOf("file_group_id");

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
		msg.isMarkable = query.value(idxIsMarkable).toBool();
		msg.isEdited = query.value(idxIsEdited).toBool();
		msg.spoilerHint = query.value(idxSpoilerHint).toString();
		msg.errorText = query.value(idxErrorText).toString();
		msg.isSpoiler = query.value(idxIsSpoiler).toBool();
		msg.replaceId = query.value(idxReplaceId).toString();
		msg.originId = query.value(idxOriginId).toString();
		msg.stanzaId = query.value(idxStanza).toString();
		msg.fileGroupId = variantToOptional<qint64>(query.value(idxFileGroupId));
		// this is useful with resending pending messages
		msg.receiptRequested = true;

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
		rec.append(createSqlField("message", newMsg.body));
	if (oldMsg.deliveryState != newMsg.deliveryState)
		rec.append(createSqlField("deliveryState", int(newMsg.deliveryState)));
	if (oldMsg.isMarkable != newMsg.isMarkable)
		rec.append(createSqlField("isMarkable", newMsg.isMarkable));
	if (oldMsg.errorText != newMsg.errorText)
		rec.append(createSqlField("errorText", newMsg.errorText));
	if (oldMsg.isEdited != newMsg.isEdited)
		rec.append(createSqlField("isEdited", newMsg.isEdited));
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
		rec.append(createSqlField("file_group_id", optionalToVariant(newMsg.fileGroupId)));
	}

	return rec;
}

QFuture<QVector<Message>> MessageDb::fetchMessages(const QString &accountJid, const QString &chatJid, int index)
{
	return run([this, accountJid, chatJid, index]() {
		auto query = createQuery();
		prepareQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " "
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

QFuture<QVector<Message> > MessageDb::fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId)
{
	return run([this, accountJid, chatJid, index, limitingId]() {
		auto query = createQuery();
		prepareQuery(
			query,
			"SELECT * FROM Messages "
			"WHERE (sender = :accountJid AND recipient = :chatJid) OR "
			"(sender = :chatJid AND recipient = :accountJid) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, ("
			"SELECT COUNT() FROM Messages "
			"WHERE timestamp >= "
			"(SELECT timestamp FROM Messages "
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

Message MessageDb::_fetchLastMessage(const QString &user1, const QString &user2)
{
	auto query = createQuery();
	execQuery(
		query,
		"SELECT * FROM " DB_TABLE_MESSAGES " "
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
		execQuery(query, "SELECT timestamp FROM messages ORDER BY timestamp DESC LIMIT 1");

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
			"SELECT timestamp FROM " DB_TABLE_MESSAGES " DESC WHERE sender = ? AND recipient = ? AND id = ? LIMIT 1",
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
			"SELECT id FROM " DB_TABLE_MESSAGES " WHERE sender = ? AND recipient = ? ORDER BY timestamp DESC LIMIT ?, 1",
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
			"SELECT COUNT(*) FROM " DB_TABLE_MESSAGES" DESC WHERE sender = ? AND recipient = ? AND "
			"datetime(timestamp) BETWEEN "
			"datetime((SELECT timestamp FROM " DB_TABLE_MESSAGES " DESC WHERE "
			"sender = ? AND recipient = ? AND id = ? LIMIT 1)) AND "
			"datetime((SELECT timestamp FROM " DB_TABLE_MESSAGES " DESC WHERE "
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
	return run([this, msg, origin]() {
		// deduplication
		switch (origin) {
		case MessageOrigin::MamBacklog:
		case MessageOrigin::MamCatchUp:
		case MessageOrigin::Stream:
			if (_checkMessageExists(msg)) {
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
			"INSERT INTO messages (sender, recipient, timestamp, message, id, encryption, "
			"senderKey, deliveryState, isMarkable, isEdited, isSpoiler, spoilerHint, "
			"errorText, replaceId, originId, stanzaId, file_group_id) "
			"VALUES (:sender, :recipient, :timestamp, :message, :id, :encryption, :senderKey, "
			":deliveryState, :isMarkable, :isEdited, :isSpoiler, :spoilerHint, :errorText, "
			":replaceId, :originId, :stanzaId, :file_group_id)"
		);

		bindValues(query, {
			{ u":sender", msg.from },
			{ u":recipient", msg.to },
			{ u":timestamp", msg.stamp.toString(Qt::ISODateWithMs) },
			{ u":message", msg.body },
			{ u":id", msg.id.isEmpty() ? " " : msg.id },
			{ u":encryption", msg.encryption },
			{ u":senderKey", msg.senderKey },
			{ u":deliveryState", int(msg.deliveryState) },
			{ u":isMarkable", msg.isMarkable },
			{ u":isEdited", msg.isEdited },
			{ u":isSpoiler", msg.isSpoiler },
			{ u":spoilerHint", msg.spoilerHint },
			{ u":errorText", msg.errorText },
			{ u":replaceId", msg.replaceId },
			{ u":originId", msg.originId },
			{ u":stanzaId", msg.stanzaId },
			{ u":file_group_id", optionalToVariant(msg.fileGroupId) }
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
			execQuery(query, "SELECT file_group_id FROM messages WHERE file_group_id IS NOT NULL");

			QVector<qint64> fileIds;
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

QFuture<void> MessageDb::updateMessage(const QString &id,
                                       const std::function<void (Message &)> &updateMsg)
{
	return run([this, id, updateMsg]() {
		emit messageUpdated(id, updateMsg);

		// load current message item from db
		auto query = createQuery();
		execQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " WHERE id = ? LIMIT 1",
			{ id }
		);

		auto msgs = _fetchMessagesFromQuery(query);
		_fetchReactions(msgs);

		// update loaded item
		if (!msgs.isEmpty()) {
			const auto &oldMessage = msgs.first();
			Message newMessage = oldMessage;
			updateMsg(newMessage);

			// Replace the old message's values with the updated ones if the message has changed.
			if (oldMessage != newMessage) {
				const auto &oldReactions = oldMessage.reactions;
				if (const auto &newReactions = newMessage.reactions; oldReactions != newReactions) {
					// Remove old reactions.
					for (auto itr = oldReactions.begin(); itr != oldReactions.end(); ++itr) {
						const auto &senderJid = itr.key();
						const auto reaction = itr.value();

						for (const auto &emoji : reaction.emojis) {
							if (!newReactions.value(senderJid).emojis.contains(emoji)) {
								execQuery(
									query,
									"DELETE FROM " DB_TABLE_MESSAGE_REACTIONS " "
									"WHERE messageSender = :messageSender AND messageRecipient = :messageRecipient AND messageId = :messageId AND senderJid = :senderJid AND emoji = :emoji",
									{ { u":messageSender", oldMessage.from },
									  { u":messageRecipient", oldMessage.to },
									  { u":messageId", oldMessage.id },
									  { u":senderJid", senderJid },
									  { u":emoji", emoji } }
								);
							}
						}
					}

					// Add new reactions.
					for (auto itr = newReactions.begin(); itr != newReactions.end(); ++itr) {
						const auto &senderJid = itr.key();
						const auto reaction = itr.value();

						for (const auto &emoji : reaction.emojis) {
							if (!oldReactions.value(senderJid).emojis.contains(emoji)) {
								execQuery(
									query,
									"INSERT INTO " DB_TABLE_MESSAGE_REACTIONS " "
									"(messageSender, messageRecipient, messageId, senderJid, timestamp, emoji) "
									"VALUES (:messageSender, :messageRecipient, :messageId, :senderJid, :timestamp, :emoji)",
									{ { u":messageSender", oldMessage.from },
									  { u":messageRecipient", oldMessage.to },
									  { u":messageId", oldMessage.id },
									  { u":senderJid", senderJid },
									  { u":timestamp", reaction.latestTimestamp },
									  { u":emoji", emoji }}
								);
							}
						}
					}
				} else if (auto rec = createUpdateRecord(oldMessage, newMessage); rec.count()) {
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
		prepareQuery(query, "INSERT OR REPLACE INTO file_hashes VALUES (?, ?, ?)");
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
		prepareQuery(query, "INSERT OR REPLACE INTO file_http_sources VALUES (?, ?)");
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
		prepareQuery(query, "INSERT OR REPLACE INTO file_encrypted_sources VALUES (?, ?, ?, ?, ?, ?)");
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
	prepareQuery(query, "DELETE FROM file_hashes WHERE data_id = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

void MessageDb::_removeHttpSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM file_http_sources WHERE file_id = ?");
	for (auto id : fileIds) {
		bindValues(query, { QVariant(id) });
		execQuery(query);
	}
}

void MessageDb::_removeEncryptedSources(const QVector<qint64> &fileIds)
{
	auto query = createQuery();
	prepareQuery(query, "DELETE FROM file_encrypted_sources WHERE file_id = ?");
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
		prepareQuery(q, "SELECT id, name, description, mime_type, size, last_modified, disposition, "
		                "thumbnail, local_file_path FROM files "
		                "WHERE file_group_id = :file_group_id");
		return q;
	}();

	bindValues(query, { QueryBindValue { u":file_group_id", QVariant(fileGroupId) } });
	execQuery(query);

	QVector<File> files;
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
		prepareQuery(q, "SELECT hash_type, hash_value FROM file_hashes WHERE data_id = ?");
		return q;
	}();

	bindValues(query, { QVariant(fileId) });
	execQuery(query);

	QVector<FileHash> hashes;
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
		prepareQuery(q, "SELECT url FROM file_http_sources WHERE file_id = ?");
		return q;
	}();

	bindValues(query, { QVariant(fileId) });
	execQuery(query);

	QVector<HttpSource> sources;
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
		prepareQuery(q, "SELECT url, cipher, key, iv, encrypted_data_id FROM file_encrypted_sources WHERE file_id = ?");
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
	enum { SenderJid, Timestamp, Emoji };
	auto query = createQuery();

	for (auto &message : messages) {
		execQuery(
			query,
			"SELECT senderJid, timestamp, emoji FROM messageReactions "
			"WHERE messageSender = :messageSender AND messageRecipient = :messageRecipient AND messageId = :messageId",
			{ { u":messageSender", message.from }, { u":messageRecipient", message.to }, { u":messageId", message.id } }
		);

		// Iterate over all found emojis.
		while (query.next()) {
			auto &reaction = message.reactions[query.value(SenderJid).toString()];

			// Use the timestamp of the current emoji as the latest timestamp if the emoji's
			// timestamp is newer than the latest one.
			if (const auto timestamp = query.value(Timestamp).toDateTime(); reaction.latestTimestamp < timestamp) {
				reaction.latestTimestamp = timestamp;
			}

			reaction.emojis.append(query.value(Emoji).toString());
		}
	}
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
		bindValues.push_back({ u":originId", message.stanzaId });
	}
	if (!message.id.isEmpty()) {
		idChecks << QStringLiteral("id = :id");
		bindValues.push_back({ u":id", message.stanzaId });
	}

	if (idChecks.isEmpty()) {
		// if we have no checks because of missing IDs, report that the message
		// does not exist
		return false;
	}

	const QString idConditionSql = idChecks.join(u" OR ");
	const QString querySql =
		QStringLiteral("SELECT COUNT(*) FROM " DB_TABLE_MESSAGES " "
		               "WHERE (sender = :from AND recipient = :to AND (") %
		idConditionSql %
		QStringLiteral(")) ORDER BY timestamp DESC LIMIT " CHECK_MESSAGE_EXISTS_DEPTH_LIMIT);

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
			"SELECT * FROM " DB_TABLE_MESSAGES " "
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
