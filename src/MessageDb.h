/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
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

#pragma once

#include <QObject>

#include "Message.h"
#include "DatabaseComponent.h"

class Database;
class QSqlQuery;
class QSqlRecord;

/**
 * @class The MessageDb is used to query the 'messages' database table. It's used by the
 * MessageModel to load messages and by the MessageHandler to insert messages.
 */
class MessageDb : public DatabaseComponent
{
	Q_OBJECT

public:
	/**
	 * @struct The MessageResult is used inside the MessageDb class to retrieve
	 * the messages as QVector as well as the queryIndex.
	 * @brief The default constructor sets queryIndex to -1 which defines that
	 * the MessageResult is empty.
	 */
	struct MessageResult {
		QVector<Message> messages;
		int queryIndex = -1;
	};

	explicit MessageDb(Database *db, QObject *parent = nullptr);
	~MessageDb();

	static MessageDb *instance();

	/**
	 * Creates an @c QSqlRecord for updating an old message to a new message.
	 *
	 * @param oldMsg Full message as it is currently saved
	 * @param newMsg Full message as it should be after the update query ran.
	 */
	static QSqlRecord createUpdateRecord(const Message &oldMsg,
	                                     const Message &newMsg);

	/**
	 * Fetches more entries from the database and emits messagesFetched() with the results.
	 *
	 * @param accountJid bare JID of the user's account
	 * @param chatJid bare Jid of the chat
	 * @param index number of entries to be skipped, used for paging
	 *
	 * @return the fetched messages
	 */
	QFuture<QVector<Message>> fetchMessages(const QString &accountJid, const QString &chatJid, int index);

	/**
	 * Fetches entries until the first message of chatJid from the database and emits
	 * messagesFetched() with the results.
	 *
	 * Entries are fetched until a message from chatJid is found.
	 * Those entries plus DB_QUERY_LIMIT_MESSAGES entries are returned.
	 * If no message from chatJid could be found, only DB_QUERY_LIMIT_MESSAGES entries are returned.
	 *
	 * @param accountJid bare JID of the user's account
	 * @param chatJid bare Jid of the chat
	 * @param index number of entries to be skipped, used for paging
	 *
	 * @return the fetched messages
	 */
	QFuture<QVector<Message>> fetchMessagesUntilFirstContactMessage(const QString &accountJid, const QString &chatJid, int index);

	/**
	 * Fetches entries until messageId from the database and emits messagesFetched() with the
	 * results.
	 *
	 * Entries are fetched until a message with messageId is found.
	 * Those entries plus DB_QUERY_LIMIT_MESSAGES entries are returned.
	 * If no message with messageId could be found, only DB_QUERY_LIMIT_MESSAGES entries are
	 * returned.
	 *
	 * @param accountJid bare JID of the user's account
	 * @param chatJid bare Jid of the chat
	 * @param index number of entries to be skipped, used for paging
	 * @param limitingId ID of the message until messages are fetched
	 *
	 * @return the fetched messages
	 */
	QFuture<QVector<Message>> fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId);

	Q_SIGNAL void messagesFetched(const QVector<Message> &messages);

	/**
	 * Fetches more entries from the database and emits messagesFetched() with the results.
	 *
	 * Entries are fetched until a message with queryString is found.
	 * Those entries plus DB_QUERY_LIMIT_MESSAGES entries are returned.
	 * If no message with queryString could be found, no messages are returned.
	 *
	 * The returned query index is -1 if no message with queryString could be found,
	 * otherwise index - 1.
	 *
	 * @param accountJid bare JID of the user's account
	 * @param chatJid bare Jid of the chat
	 * @param index number of entries to be skipped, used for paging
	 * @param queryString string to be queried
	 *
	 * @return the fetched messages and the found message index - 1
	 */
	QFuture<MessageResult> fetchMessagesUntilQueryString(const QString &accountJid, const QString &chatJid, int index, const QString &queryString);

	/**
	 * @brief Fetches messages that are marked as pending.
	 *
	 * @param userJid JID of the user whose messages should be fetched
	 */
	QFuture<QVector<Message>> fetchPendingMessages(const QString &userJid);
	Q_SIGNAL void pendingMessagesFetched(const QVector<Message> &messages);

	/**
	 * Fetches the last message and returns it.
	 */
	Message _fetchLastMessage(const QString &user1, const QString &user2);

	/**
	 * Fetch the latest message stamp
	 */
	QFuture<QDateTime> fetchLastMessageStamp();
	Q_SIGNAL void lastMessageStampFetched(const QDateTime &stamp);

	/**
	 * Returns the timestamp of a message.
	 *
	 * @param senderJid JID of the message's sender
	 * @param recipientJid JID of the message's recipient
	 * @param messageId ID of the message
	 *
	 * @return the message's timestamp
	 */
	QFuture<QDateTime> messageTimestamp(const QString &senderJid, const QString &recipientJid, const QString &messageId);

	QFuture<QString> firstContactMessageId(const QString &accountJid, const QString &chatJid, int index);

	/**
	 * Returns the count of messages chronologically between (including) two given messages
	 * specified by their IDs.
	 *
	 * @param senderJid JID of the messages' sender
	 * @param recipientJid JID of the messages' recipient
	 * @param messageIdBegin ID of the message to start the counting
	 * @param messageIdEnd ID of the message to end the counting
	 *
	 * @return the message count
	 */
	QFuture<int> messageCount(const QString &senderJid, const QString &recipientJid, const QString &messageIdBegin, const QString &messageIdEnd);

	/**
	 * Adds a message to the database.
	 */
	QFuture<void> addMessage(const Message &msg, MessageOrigin origin);
	Q_SIGNAL void messageAdded(const Message &msg, MessageOrigin origin);

	/**
	 * Removes all messages of an account or an account's chat.
	 *
	 * @param accountJid JID of the account whose messages are being removed
	 * @param chatJid JID of the chat whose messages are being removed (optional)
	 */
	QFuture<void> removeMessages(const QString &accountJid, const QString &chatJid = {});

	/**
	 * Loads a message, runs the update lambda and writes it to the DB again.
	 *
	 * @param updateMsg Function that changes the message
	 */
	QFuture<void> updateMessage(const QString &id, const std::function<void (Message &)> &updateMsg);
	Q_SIGNAL void messageUpdated(const QString &id, const std::function<void (Message &)> &updateMsg);

	/**
	 * Adds a draft message to the database.
	 */
	QFuture<Message> addDraftMessage(const Message &msg);
	Q_SIGNAL void draftMessageAdded(const Message &msg);

	/**
	 * Updates a draft message from the database.
	 */
	QFuture<Message> updateDraftMessage(const Message &msg);
	Q_SIGNAL void draftMessageUpdated(const Message &msg);

	/**
	 * Removes a draft message from the database.
	 */
	QFuture<QString> removeDraftMessage(const QString &id);
	Q_SIGNAL void draftMessageRemoved(const QString &id);

	/**
	 * Fetch a draft message from the database.
	 */
	QFuture<Message> fetchDraftMessage(const QString &id);
	Q_SIGNAL void draftMessageFetched(const Message &msg);

private:
	// Setters do INSERT OR REPLACE INTO
	void _setFiles(const QVector<File> &files);
	void _setFileHashes(const QVector<FileHash> &fileHashes);
	void _setHttpSources(const QVector<HttpSource> &sources);
	void _setEncryptedSources(const QVector<EncryptedSource> &sources);
	void _removeFiles(const QVector<qint64> &fileIds);
	void _removeFileHashes(const QVector<qint64> &fileIds);
	void _removeHttpSources(const QVector<qint64> &fileIds);
	void _removeEncryptedSources(const QVector<qint64> &fileIds);
	QVector<Message> _fetchMessagesFromQuery(QSqlQuery &query);
	QVector<File> _fetchFiles(qint64 fileGroupId);
	QVector<FileHash> _fetchFileHashes(qint64 dataId);
	QVector<HttpSource> _fetchHttpSource(qint64 fileId);
	QVector<EncryptedSource> _fetchEncryptedSource(qint64 fileId);

	void _fetchReactions(QVector<Message> &messages);

	/**
	 * Checks whether a message already exists in the database
	 */
	bool _checkMessageExists(const Message &message);

	static MessageDb *s_instance;
};
