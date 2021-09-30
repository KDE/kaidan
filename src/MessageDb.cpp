/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2021 Kaidan developers and contributors
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

void MessageDb::parseMessagesFromQuery(QSqlQuery &query, QVector<Message> &msgs)
{
	// get indexes of attributes
	QSqlRecord rec = query.record();
	int idxFrom = rec.indexOf("author");
	int idxTo = rec.indexOf("recipient");
	int idxStamp = rec.indexOf("timestamp");
	int idxId = rec.indexOf("id");
	int idxBody = rec.indexOf("message");
	int idxDeliveryState = rec.indexOf("deliveryState");
	int idxMediaType = rec.indexOf("type");
	int idxOutOfBandUrl = rec.indexOf("mediaUrl");
	int idxMediaContentType = rec.indexOf("mediaContentType");
	int idxMediaLocation = rec.indexOf("mediaLocation");
	int idxMediaSize = rec.indexOf("mediaSize");
	int idxMediaLastModified = rec.indexOf("mediaLastModified");
	int idxIsEdited = rec.indexOf("edited");
	int idxSpoilerHint = rec.indexOf("spoilerHint");
	int idxIsSpoiler = rec.indexOf("isSpoiler");
	int idxErrorText = rec.indexOf("errorText");
	int idxReplaceId = rec.indexOf("replaceId");
	int idxOriginId = rec.indexOf("originId");
	int idxStanza = rec.indexOf("stanzaId");

	while (query.next()) {
		Message msg;
		msg.setFrom(query.value(idxFrom).toString());
		msg.setTo(query.value(idxTo).toString());
		msg.setStamp(QDateTime::fromString(
			query.value(idxStamp).toString(),
			Qt::ISODate
		));
		msg.setId(query.value(idxId).toString());
		msg.setBody(query.value(idxBody).toString());
		msg.setDeliveryState(static_cast<Enums::DeliveryState>(query.value(idxDeliveryState).toInt()));
		msg.setMediaType(static_cast<MessageType>(query.value(idxMediaType).toInt()));
		msg.setOutOfBandUrl(query.value(idxOutOfBandUrl).toString());
		msg.setMediaContentType(query.value(idxMediaContentType).toString());
		msg.setMediaLocation(query.value(idxMediaLocation).toString());
		msg.setMediaSize(query.value(idxMediaSize).toLongLong());
		msg.setMediaLastModified(QDateTime::fromMSecsSinceEpoch(
			query.value(idxMediaLastModified).toLongLong()
		));
		msg.setIsEdited(query.value(idxIsEdited).toBool());
		msg.setSpoilerHint(query.value(idxSpoilerHint).toString());
		msg.setErrorText(query.value(idxErrorText).toString());
		msg.setIsSpoiler(query.value(idxIsSpoiler).toBool());
		msg.setReplaceId(query.value(idxReplaceId).toString());
		msg.setOriginId(query.value(idxOriginId).toString());
		msg.setStanzaId(query.value(idxStanza).toString());
		msg.setReceiptRequested(true);	//this is useful with resending pending messages
		msgs << msg;
	}
}

QSqlRecord MessageDb::createUpdateRecord(const Message &oldMsg, const Message &newMsg)
{
	QSqlRecord rec;

	if (oldMsg.from() != newMsg.from())
		rec.append(SqlUtils::createSqlField("author", newMsg.from()));
	if (oldMsg.to() != newMsg.to())
		rec.append(SqlUtils::createSqlField("recipient", newMsg.to()));
	if (oldMsg.stamp() != newMsg.stamp())
		rec.append(SqlUtils::createSqlField(
		        "timestamp",
		        newMsg.stamp().toString(Qt::ISODate)
		));
	if (oldMsg.id() != newMsg.id()) {
		// TODO: remove as soon as 'NOT NULL' was removed from id column
		if (newMsg.id().isEmpty())
			rec.append(SqlUtils::createSqlField("id", QStringLiteral(" ")));
		else
			rec.append(SqlUtils::createSqlField("id", newMsg.id()));
	}
	if (oldMsg.body() != newMsg.body())
		rec.append(SqlUtils::createSqlField("message", newMsg.body()));
	if (oldMsg.deliveryState() != newMsg.deliveryState())
		rec.append(SqlUtils::createSqlField("deliveryState", int(newMsg.deliveryState())));
	if (oldMsg.errorText() != newMsg.errorText())
		rec.append(SqlUtils::createSqlField("errorText", newMsg.errorText()));
	if (oldMsg.mediaType() != newMsg.mediaType())
		rec.append(SqlUtils::createSqlField("type", int(newMsg.mediaType())));
	if (oldMsg.outOfBandUrl() != newMsg.outOfBandUrl())
		rec.append(SqlUtils::createSqlField("mediaUrl", newMsg.outOfBandUrl()));
	if (oldMsg.mediaContentType() != newMsg.mediaContentType())
		rec.append(SqlUtils::createSqlField(
		        "mediaContentType",
		        newMsg.mediaContentType()
		));
	if (oldMsg.mediaLocation() != newMsg.mediaLocation())
		rec.append(SqlUtils::createSqlField(
		         "mediaLocation",
		         newMsg.mediaLocation()
		));
	if (oldMsg.mediaSize() != newMsg.mediaSize())
		rec.append(SqlUtils::createSqlField("mediaSize", newMsg.mediaSize()));
	if (oldMsg.mediaLastModified() != newMsg.mediaLastModified())
		rec.append(SqlUtils::createSqlField(
			"mediaLastModified",
			newMsg.mediaLastModified().toMSecsSinceEpoch()
		));
	if (oldMsg.isEdited() != newMsg.isEdited())
		rec.append(SqlUtils::createSqlField("edited", newMsg.isEdited()));
	if (oldMsg.spoilerHint() != newMsg.spoilerHint())
		rec.append(SqlUtils::createSqlField("spoilerHint", newMsg.spoilerHint()));
	if (oldMsg.isSpoiler() != newMsg.isSpoiler())
		rec.append(SqlUtils::createSqlField("isSpoiler", newMsg.isSpoiler()));
	if (oldMsg.replaceId() != newMsg.replaceId())
		rec.append(SqlUtils::createSqlField("replaceId", newMsg.replaceId()));
	if (oldMsg.originId() != newMsg.originId())
		rec.append(SqlUtils::createSqlField("originId", newMsg.originId()));
	if (oldMsg.stanzaId() != newMsg.stanzaId())
		rec.append(SqlUtils::createSqlField("stanzaId", newMsg.stanzaId()));

	return rec;
}

QFuture<QVector<Message>> MessageDb::fetchMessages(const QString &user1, const QString &user2, int index)
{
	return run([this, user1, user2, index]() {
		auto query = createQuery();

		QMap<QString, QVariant> bindValues = {
			{":user1", user1},
			{":user2", user2},
			{":index", index},
			{":limit", DB_QUERY_LIMIT_MESSAGES},
		};

		SqlUtils::execQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " "
			"WHERE (author = :user1 AND recipient = :user2) OR "
			      "(author = :user2 AND recipient = :user1) "
			"ORDER BY timestamp DESC "
			"LIMIT :index, :limit",
			bindValues
		);

		QVector<Message> messages;
		parseMessagesFromQuery(query, messages);

		emit messagesFetched(messages);
		return messages;
	});
}

Message MessageDb::_fetchLastMessage(const QString &user1, const QString &user2)
{
	auto query = createQuery();

	QMap<QString, QVariant> bindValues = {
		{ QStringLiteral(":user1"), user1 },
		{ QStringLiteral(":user2"), user2 },
	};

	SqlUtils::execQuery(
		query,
		"SELECT * FROM " DB_TABLE_MESSAGES " "
		"WHERE (author = :user1 AND recipient = :user2) OR "
		      "(author = :user2 AND recipient = :user1) "
		"ORDER BY timestamp DESC "
		"LIMIT 1",
		bindValues
	);

	QVector<Message> messages;
	parseMessagesFromQuery(query, messages);

	if (!messages.isEmpty())
		return messages.first();
	return {};
}

QFuture<QDateTime> MessageDb::fetchLastMessageStamp()
{
	return run([this]() {
		auto query = createQuery();
		SqlUtils::execQuery(query, "SELECT timestamp FROM Messages ORDER BY timestamp DESC LIMIT 1");

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

		QSqlRecord record = sqlRecord(DB_TABLE_MESSAGES);
		record.setValue("author", msg.from());
		record.setValue("recipient", msg.to());
		record.setValue("timestamp", msg.stamp().toString(Qt::ISODate));
		record.setValue("message", msg.body());
		record.setValue("id", msg.id().isEmpty() ? " " : msg.id());
		record.setValue("deliveryState", int(msg.deliveryState()));
		record.setValue("type", int(msg.mediaType()));
		record.setValue("edited", msg.isEdited());
		record.setValue("isSpoiler", msg.isSpoiler());
		record.setValue("spoilerHint", msg.spoilerHint());
		record.setValue("mediaUrl", msg.outOfBandUrl());
		record.setValue("mediaContentType", msg.mediaContentType());
		record.setValue("mediaLocation", msg.mediaLocation());
		record.setValue("mediaSize", msg.mediaSize());
		record.setValue("mediaLastModified", msg.mediaLastModified().toMSecsSinceEpoch());
		record.setValue("errorText", msg.errorText());
		record.setValue("replaceId", msg.replaceId());
		record.setValue("originId", msg.originId());
		record.setValue("stanzaId", msg.stanzaId());

		auto query = createQuery();
		SqlUtils::execQuery(query, sqlDriver().sqlStatement(
				QSqlDriver::InsertStatement,
				DB_TABLE_MESSAGES,
				record,
				false
		));
	});
}

QFuture<void> MessageDb::removeMessages(const QString &, const QString &)
{
	return run([this]() {
		auto query = createQuery();
		SqlUtils::execQuery(query, "DELETE FROM " DB_TABLE_MESSAGES);
	});
}

QFuture<void> MessageDb::updateMessage(const QString &id,
                                       const std::function<void (Message &)> &updateMsg)
{
	return run([this, id, updateMsg]() {
		// load current message item from db
		auto query = createQuery();
		SqlUtils::execQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " WHERE id = ? LIMIT 1",
			QVector<QVariant> { id }
		);

		QVector<Message> msgs;
		parseMessagesFromQuery(query, msgs);

		// update loaded item
		if (!msgs.isEmpty()) {
			Message msg = msgs.first();
			updateMsg(msg);

			// replace old message with updated one, if message has changed
			if (msgs.first() != msg) {
				// create an SQL record with only the differences
				QSqlRecord rec = createUpdateRecord(msgs.first(), msg);
				auto &driver = sqlDriver();

				SqlUtils::execQuery(
					query,
					driver.sqlStatement(
						QSqlDriver::UpdateStatement,
						DB_TABLE_MESSAGES,
						rec,
						false
					) +
					SqlUtils::simpleWhereStatement(&driver, "id", id)
				);
			}
		}
	});
}

bool MessageDb::_checkMessageExists(const Message &message)
{
	QMap<QString, QVariant> bindValues = {
		{ ":to", message.to() },
		{ ":from", message.from() },
	};

	// Check which IDs to check
	QStringList idChecks;
	if (!message.stanzaId().isEmpty()) {
		idChecks << QStringLiteral("stanzaId = :stanzaId");
		bindValues.insert(QStringLiteral(":stanzaId"), message.stanzaId());
	}
	// only check origin IDs if the message was possibly sent by us (since
	// Kaidan uses random suffixes in the resource, we can't check the resource)
	if (message.isOwn() && !message.originId().isEmpty()) {
		idChecks << QStringLiteral("originId = :originId");
		bindValues.insert(QStringLiteral(":originId"), message.stanzaId());
	}
	if (!message.id().isEmpty()) {
		idChecks << QStringLiteral("id = :id");
		bindValues.insert(QStringLiteral(":id"), message.stanzaId());
	}

	if (idChecks.isEmpty()) {
		// if we have no checks because of missing IDs, report that the message
		// does not exist
		return false;
	}

	const QString idConditionSql = idChecks.join(u" OR ");
	const QString querySql =
		QStringLiteral("SELECT COUNT(*) FROM " DB_TABLE_MESSAGES " "
		               "WHERE (author = :from AND recipient = :to AND (") %
		idConditionSql %
		QStringLiteral(")) ORDER BY timestamp DESC LIMIT " CHECK_MESSAGE_EXISTS_DEPTH_LIMIT);

	auto query = createQuery();
	SqlUtils::execQuery(query, querySql, bindValues);

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

		QMap<QString, QVariant> bindValues = {
			{":user", userJid},
			{":deliveryState", int(Enums::DeliveryState::Pending)},
		};

		SqlUtils::execQuery(
			query,
			"SELECT * FROM " DB_TABLE_MESSAGES " "
			"WHERE (author = :user AND deliveryState = :deliveryState) "
			"ORDER BY timestamp ASC",
			bindValues
		);

		QVector<Message> messages;
		parseMessagesFromQuery(query, messages);

		emit pendingMessagesFetched(messages);
		return messages;
	});
}

