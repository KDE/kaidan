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
// Kaidan
#include "Database.h"
#include "Globals.h"
#include "SqlUtils.h"

using namespace SqlUtils;

#define CHECK_MESSAGE_EXISTS_DEPTH_LIMIT "20"

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
	int idxOutOfBandUrl = rec.indexOf("mediaUrl");
	int idxMediaLocation = rec.indexOf("mediaLocation");
	int idxIsEdited = rec.indexOf("isEdited");
	int idxSpoilerHint = rec.indexOf("spoilerHint");
	int idxIsSpoiler = rec.indexOf("isSpoiler");
	int idxErrorText = rec.indexOf("errorText");
	int idxReplaceId = rec.indexOf("replaceId");
	int idxOriginId = rec.indexOf("originId");
	int idxStanza = rec.indexOf("stanzaId");

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
		msg.outOfBandUrl = query.value(idxOutOfBandUrl).toString();
		msg.mediaLocation = query.value(idxMediaLocation).toString();
		msg.isEdited = query.value(idxIsEdited).toBool();
		msg.spoilerHint = query.value(idxSpoilerHint).toString();
		msg.errorText = query.value(idxErrorText).toString();
		msg.isSpoiler = query.value(idxIsSpoiler).toBool();
		msg.replaceId = query.value(idxReplaceId).toString();
		msg.originId = query.value(idxOriginId).toString();
		msg.stanzaId = query.value(idxStanza).toString();
		// this is useful with resending pending messages
		msg.receiptRequested = true;

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
	if (oldMsg.outOfBandUrl != newMsg.outOfBandUrl)
		rec.append(createSqlField("mediaUrl", newMsg.outOfBandUrl));
	if (oldMsg.mediaLocation != newMsg.mediaLocation)
		rec.append(createSqlField(
			"mediaLocation",
			newMsg.mediaLocation
		));
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

	return rec;
}

QFuture<QVector<Message>> MessageDb::fetchMessages(const QString &user1, const QString &user2, int index)
{
	return run([this, user1, user2, index]() {
		auto query = createQuery();
		prepareQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " "
			"WHERE (sender = :user1 AND recipient = :user2) OR "
			      "(sender = :user2 AND recipient = :user1) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, :limit"
		);
		bindValues(query, {
			{ u":user1", user1 },
			{ u":user2", user2 },
			{ u":index", index },
			{ u":limit", DB_QUERY_LIMIT_MESSAGES },
		});
		execQuery(query);

		auto messages = _fetchMessagesFromQuery(query);

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
			"errorText, replaceId, originId, stanzaId, mediaUrl, mediaLocation) "
			"VALUES (:sender, :recipient, :timestamp, :message, :id, :encryption, :senderKey, "
			":deliveryState, :isMarkable, :isEdited, :isSpoiler, :spoilerHint, :errorText, "
			":replaceId, :originId, :stanzaId, :mediaUrl, :mediaLocation)"
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
			{ u":mediaUrl", msg.outOfBandUrl },
			{ u":mediaLocation", msg.mediaLocation },
			{ u":errorText", msg.errorText },
			{ u":replaceId", msg.replaceId },
			{ u":originId", msg.originId },
			{ u":stanzaId", msg.stanzaId },
		});
		execQuery(query);
	});
}

QFuture<void> MessageDb::removeMessages(const QString &, const QString &)
{
	return run([this]() {
		auto query = createQuery();
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

		// update loaded item
		if (!msgs.isEmpty()) {
			Message msg = msgs.first();
			updateMsg(msg);

			// replace old message with updated one, if message has changed
			if (msgs.first() != msg) {
				// create an SQL record with only the differences
				QSqlRecord rec = createUpdateRecord(msgs.first(), msg);
				auto &driver = sqlDriver();

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
		}
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
