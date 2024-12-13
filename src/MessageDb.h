// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

#include "DatabaseComponent.h"
#include "Message.h"
#include "SqlUtils.h"

class Database;
class QSqlQuery;
class QSqlRecord;

using namespace SqlUtils;

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

    qint64 newFileId();
    qint64 newFileGroupId();

    /**
     * Creates an @c QSqlRecord for updating an old message to a new message.
     *
     * @param oldMsg Full message as it is currently saved
     * @param newMsg Full message as it should be after the update query ran.
     */
    static QSqlRecord createUpdateRecord(const Message &oldMsg, const Message &newMsg);

    /**
     * Fetches messages.
     *
     * @param accountJid bare JID of the user's account
     * @param chatJid bare Jid of the chat
     * @param index number of entries to be skipped, used for paging
     *
     * @return the fetched messages
     */
    QFuture<QVector<Message>> fetchMessages(const QString &accountJid, const QString &chatJid, int index);

    /**
     * Fetches shared media for an account from the database.
     *
     * @param accountJid bare JID of the user's account
     *
     * @return the fetched messages
     */
    QFuture<QVector<File>> fetchFiles(const QString &accountJid);

    /**
     * Fetches shared media of a chat from the database.
     *
     * @param accountJid bare JID of the user's account
     * @param chatJid bare Jid of the chat
     *
     * @return the fetched messages
     */
    QFuture<QVector<File>> fetchFiles(const QString &accountJid, const QString &chatJid);

    /**
     * Fetches downloaded shared media of an account from the database.
     *
     * @param accountJid bare JID of the user's account
     *
     * @return the fetched messages
     */
    QFuture<QVector<File>> fetchDownloadedFiles(const QString &accountJid);

    /**
     * Fetches downloaded shared media of a chat from the database.
     *
     * @param accountJid bare JID of the user's account
     * @param chatJid bare Jid of the chat
     *
     * @return the fetched messages
     */
    QFuture<QVector<File>> fetchDownloadedFiles(const QString &accountJid, const QString &chatJid);

    /**
     * Fetches messages until the first message of a specific chat.
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
     * Fetches messages until a specific ID.
     *
     * Entries are fetched until a message with messageId is found.
     * Those entries plus DB_QUERY_LIMIT_MESSAGES entries are returned.
     *
     * @param accountJid bare JID of the user's account
     * @param chatJid bare Jid of the chat
     * @param index number of entries to be skipped, used for paging
     * @param limitingId ID of the message until messages are fetched
     * @param fetchMessageMinimum whether to return a minimum of DB_QUERY_LIMIT_MESSAGES entries if
     *        no message with messageId could be found
     *
     * @return the fetched messages and the found message's index
     */
    QFuture<MessageResult>
    fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId, bool fetchMessageMinimum = true);

    /**
     * Fetches messages until a message with a specific query string.
     *
     * Entries are fetched until a message with queryString is found.
     * Those entries plus DB_QUERY_LIMIT_MESSAGES entries are returned.
     * If no message with queryString could be found, no messages are returned.
     *
     * The returned query index is -1 if no message with queryString could be found,
     * otherwise the found message's index.
     *
     * @param accountJid bare JID of the user's account
     * @param chatJid bare Jid of the chat
     * @param index number of entries to be skipped, used for paging
     * @param queryString string to be queried
     *
     * @return the fetched messages and the found message's index
     */
    QFuture<MessageResult> fetchMessagesUntilQueryString(const QString &accountJid, const QString &chatJid, int index, const QString &queryString);

    /**
     * Fetches messages that are marked as pending.
     *
     * @param accountJid JID of the account whose messages should be fetched
     */
    QFuture<QVector<Message>> fetchPendingMessages(const QString &accountJid);

    /**
     * Fetches message reactions marked as pending.
     *
     * @param accountJid JID of the account whose message reactions are fetched
     *
     * @return the IDs of the chats mapped to their reactions (message IDs mapped to the message
     *         reaction senders)
     */
    QFuture<QMap<QString, QMap<QString, MessageReactionSender>>> fetchPendingReactions(const QString &accountJid);

    /**
     * Fetches the last message from the database synchronously.
     */
    Message _fetchLastMessage(const QString &accountJid, const QString &chatJid);

    QFuture<QString> firstContactMessageId(const QString &accountJid, const QString &chatJid, int index);

    bool _hasMessage(const QString &accountJid, const QString &chatJid, const QString &groupChatSenderId);

    /**
     * Returns the count of messages chronologically between (including) two given messages
     * specified by their IDs.
     *
     * @param accountJid JID of the account
     * @param chatJid JID of the chat
     * @param messageIdBegin ID of the message to start the counting
     * @param messageIdEnd ID of the message to end the counting
     *
     * @return the message count
     */
    QFuture<int> messageCount(const QString &accountJid, const QString &chatJid, const QString &messageIdBegin, const QString &messageIdEnd);

    /**
     * Adds a message to the database.
     */
    QFuture<void> addMessage(const Message &message, MessageOrigin origin);
    Q_SIGNAL void messageAdded(const Message &msg, MessageOrigin origin);

    /**
     * Updates a stored message or adds it if it is not stored.
     */
    QFuture<void> addOrUpdateMessage(const Message &message, MessageOrigin origin, const QString &originId, const std::function<void(Message &)> &updateMsg);

    /**
     * Removes all messages from an account.
     *
     * @param accountJid JID of the account whose messages are being removed
     */
    QFuture<void> removeAllMessagesFromAccount(const QString &accountJid);
    Q_SIGNAL void allMessagesRemovedFromAccount(const QString &accountJid);

    /**
     * Removes all messages from an account's chat.
     *
     * @param accountJid JID of the account whose messages are being removed
     * @param chatJid JID of the chat whose messages are being removed
     */
    QFuture<void> removeAllMessagesFromChat(const QString &accountJid, const QString &chatJid);
    Q_SIGNAL void allMessagesRemovedFromChat(const QString &accountJid, const QString &chatJid);

    /**
     * Removes a chat message locally.
     *
     * @param accountJid JID of account
     * @param chatJid JID of the chat
     * @param messageId ID of the message
     */
    QFuture<void> removeMessage(const QString &accountJid, const QString &chatJid, const QString &messageId);
    Q_SIGNAL void messageRemoved(const Message &newLastMessage);

    /**
     * Loads a message, runs the update lambda and writes it to the DB again.
     *
     * A message can be found by its regular "id" or by its "replaceId" passed as the parameter
     * "id".
     *
     * @param id ID of the message to be updated
     * @param updateMsg Function that changes the message
     */
    QFuture<void> updateMessage(const QString &id, const std::function<void(Message &)> &updateMsg);
    Q_SIGNAL void messageUpdated(const Message &message);

    /**
     * Adds additional file sources to a file.
     *
     * The DB file IDs in the sources are overriden and may e.g. be set to 0 if unknown.
     * The whole message is fetched and messageUpdated() is emitted.
     */
    QFuture<void> attachFileSources(const QString &accountJid,
                                    const QString &chatJid,
                                    const QString &messageId,
                                    const QString &fileId,
                                    const QVector<HttpSource> &httpSources,
                                    const QVector<EncryptedSource> &encryptedSources);

    /**
     * Fetches a draft message from the database.
     */
    QFuture<std::optional<Message>> fetchDraftMessage(const QString &accountJid, const QString &chatJid);

    /**
     * Adds a draft message to the database.
     */
    QFuture<void> addDraftMessage(const Message &msg);
    Q_SIGNAL void draftMessageAdded(const Message &msg);

    /**
     * Updates a draft message from the database.
     */
    QFuture<void> updateDraftMessage(const QString &accountJid, const QString &chatJid, const std::function<void(Message &)> &updateMessage);
    Q_SIGNAL void draftMessageUpdated(const Message &msg);

    /**
     * Removes a draft message from the database.
     */
    QFuture<void> removeDraftMessage(const QString &accountJid, const QString &chatJid);
    Q_SIGNAL void draftMessageRemoved(const Message &newLastMessage);

private:
    void _addMessage(Message message, MessageOrigin origin);
    void _addMessage(const Message &message);
    void _updateMessage(const QString &id, const std::function<void(Message &)> &updateMsg);

    // Setters do INSERT OR REPLACE INTO
    void _fetchLatestFileId();
    void _fetchLatestFileGroupId();
    void _setFiles(const QVector<File> &files);
    void _setFileHashes(const QVector<FileHash> &fileHashes);
    void _setHttpSources(const QVector<HttpSource> &sources);
    void _setEncryptedSources(const QVector<EncryptedSource> &sources);
    void _removeFiles(const QVector<qint64> &fileIds);
    void _removeFileHashes(const QVector<qint64> &fileIds);
    void _removeHttpSources(const QVector<qint64> &fileIds);
    void _removeEncryptedSources(const QVector<qint64> &fileIds);
    QVector<Message> _fetchMessagesFromQuery(QSqlQuery &query);
    QVector<File> _fetchFiles(const QString &accountJid);
    QVector<File> _fetchFiles(const QString &accountJid, const QString &chatJid);
    QVector<File> _fetchFiles(const QString &statement, const QueryBindValues &bindValues);
    void _extractDownloadedFiles(QVector<File> &files);
    QVector<File> _fetchFiles(qint64 fileGroupId);
    QVector<FileHash> _fetchFileHashes(qint64 dataId);
    QVector<HttpSource> _fetchHttpSource(qint64 fileId);
    QVector<EncryptedSource> _fetchEncryptedSource(qint64 fileId);

    void _fetchAdditionalData(QVector<Message> &messages);
    void _fetchReactions(QVector<Message> &messages);
    void _fetchGroupChatUsers(QVector<Message> &messages);
    void _fetchGroupChatUser(Message &message);
    void _fetchTrustLevels(QVector<Message> &messages);
    void _fetchTrustLevel(Message &messages);

    void _fetchReply(Message &message);

    std::optional<Message> _fetchDraftMessage(const QString &accountJid, const QString &chatJid);

    /**
     * Checks whether a message already exists in the database
     */
    bool _checkMessageExists(const Message &message);

    Message _initializeLastMessage(const QString &accountJid, const QString &chatJid);

    qint64 m_latestFileId = -1;
    qint64 m_latestFileGroupId = -1;

    static MessageDb *s_instance;
};
