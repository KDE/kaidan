// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <dev@taibsu.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageDb.h"

// Qt
#include <QBuffer>
#include <QFile>
#include <QMimeDatabase>
#include <QSqlDriver>
#include <QSqlField>
#include <QSqlRecord>
#include <QStringBuilder>
#include <QThread>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Algorithms.h"
#include "Database.h"
#include "Globals.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "KaidanCoreLog.h"
#include "TrustDb.h"

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

MessageDb::MessageDb(QObject *parent)
    : DatabaseComponent(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;

    fetchLatestFileData();
}

MessageDb::~MessageDb()
{
    s_instance = nullptr;
}

MessageDb *MessageDb::instance()
{
    return s_instance;
}

qint64 MessageDb::newFileId()
{
    // Ensure that this method is only called by the main thread.
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    return ++m_latestFileId;
}

qint64 MessageDb::newFileGroupId()
{
    // Ensure that this method is only called by the main thread.
    Q_ASSERT(QThread::currentThread() == qApp->thread());
    return ++m_latestFileGroupId;
}

QList<Message> MessageDb::_fetchMessagesFromQuery(QSqlQuery &query)
{
    QList<Message> messages;

    // get indexes of attributes
    QSqlRecord rec = query.record();
    int idxAccountJid = rec.indexOf(QStringLiteral("accountJid"));
    int idxChatJid = rec.indexOf(QStringLiteral("chatJid"));
    int idxIsOwn = rec.indexOf(QStringLiteral("isOwn"));
    int idxGroupChatSenderId = rec.indexOf(QStringLiteral("groupChatSenderId"));
    int idxId = rec.indexOf(QStringLiteral("id"));
    int idxOriginId = rec.indexOf(QStringLiteral("originId"));
    int idxStanzaId = rec.indexOf(QStringLiteral("stanzaId"));
    int idxReplaceId = rec.indexOf(QStringLiteral("replaceId"));
    int idxReplyTo = rec.indexOf(QStringLiteral("replyTo"));
    int idxReplyId = rec.indexOf(QStringLiteral("replyId"));
    int idxReplyQuote = rec.indexOf(QStringLiteral("replyQuote"));
    int idxTimestamp = rec.indexOf(QStringLiteral("timestamp"));
    int idxBody = rec.indexOf(QStringLiteral("body"));
    int idxEncryption = rec.indexOf(QStringLiteral("encryption"));
    int idxSenderKey = rec.indexOf(QStringLiteral("senderKey"));
    int idxDeliveryState = rec.indexOf(QStringLiteral("deliveryState"));
    int idxIsSpoiler = rec.indexOf(QStringLiteral("isSpoiler"));
    int idxSpoilerHint = rec.indexOf(QStringLiteral("spoilerHint"));
    int idxFileGroupId = rec.indexOf(QStringLiteral("fileGroupId"));
    int idxGroupChatInviterJid = rec.indexOf(QStringLiteral("groupChatInviterJid"));
    int idxGroupChatInviteeJid = rec.indexOf(QStringLiteral("groupChatInviteeJid"));
    int idxGroupChatInvitationJid = rec.indexOf(QStringLiteral("groupChatInvitationJid"));
    int idxGroupChatToken = rec.indexOf(QStringLiteral("groupChatToken"));
    int idxErrorText = rec.indexOf(QStringLiteral("errorText"));
    int idxMarked = rec.indexOf(QStringLiteral("marked"));
    int idxRemoved = rec.indexOf(QStringLiteral("removed"));

    reserve(messages, query);
    while (query.next()) {
        Message msg;
        msg.accountJid = query.value(idxAccountJid).toString();
        msg.chatJid = query.value(idxChatJid).toString();
        msg.isOwn = query.value(idxIsOwn).toBool();
        msg.groupChatSenderId = query.value(idxGroupChatSenderId).toString();
        msg.id = query.value(idxId).toString();
        msg.originId = query.value(idxOriginId).toString();
        msg.stanzaId = query.value(idxStanzaId).toString();
        msg.replaceId = query.value(idxReplaceId).toString();

        if (const auto replyId = query.value(idxReplyId).toString(); !replyId.isEmpty()) {
            Message::Reply reply;
            const auto replyTo = query.value(idxReplyTo).toString();

            if (msg.isGroupChatMessage()) {
                if (const auto groupChatUser = GroupChatUserDb::instance()->_user(msg.accountJid, msg.chatJid, replyTo)) {
                    reply.toJid = groupChatUser->jid;
                    reply.toGroupChatParticipantName = groupChatUser->name;
                }

                reply.toGroupChatParticipantId = replyTo;
            } else {
                reply.toJid = replyTo;
            }

            reply.id = query.value(idxReplyId).toString();
            reply.quote = query.value(idxReplyQuote).toString();

            msg.reply = reply;
        }

        msg.timestamp = QDateTime::fromString(query.value(idxTimestamp).toString(), Qt::ISODate);
        msg.setPreparedBody(query.value(idxBody).toString());
        msg.encryption = query.value(idxEncryption).value<Encryption::Enum>();
        msg.senderKey = query.value(idxSenderKey).toByteArray();
        msg.deliveryState = query.value(idxDeliveryState).value<Enums::DeliveryState>();
        msg.isSpoiler = query.value(idxIsSpoiler).toBool();
        msg.spoilerHint = query.value(idxSpoilerHint).toString();
        msg.fileGroupId = variantToOptional<qint64>(query.value(idxFileGroupId));
        if (msg.fileGroupId) {
            msg.files = _fetchFiles(*msg.fileGroupId);
        }
        if (const auto groupChatInviterJid = query.value(idxGroupChatInviterJid).toString(); !groupChatInviterJid.isEmpty()) {
            msg.groupChatInvitation = {
                .inviterJid = groupChatInviterJid,
                .inviteeJid = query.value(idxGroupChatInviteeJid).toString(),
                .groupChatJid = query.value(idxGroupChatInvitationJid).toString(),
                .token = query.value(idxGroupChatToken).toString(),
            };
        }
        msg.errorText = query.value(idxErrorText).toString();
        msg.marked = query.value(idxMarked).toBool();
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
    if (oldMsg.isOwn != newMsg.isOwn) {
        rec.append(createSqlField(QStringLiteral("isOwn"), newMsg.isOwn));
    }
    if (oldMsg.groupChatSenderId != newMsg.groupChatSenderId) {
        rec.append(createSqlField(QStringLiteral("groupChatSenderId"), newMsg.groupChatSenderId));
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
        rec.append(createSqlStringField(QStringLiteral("replaceId"), newMsg.replaceId));
    }
    if (const auto reply = newMsg.reply; oldMsg.reply != reply) {
        if (reply) {
            rec.append(
                createSqlStringField(QStringLiteral("replyTo"), newMsg.isGroupChatMessage() ? newMsg.reply->toGroupChatParticipantId : newMsg.reply->toJid));
            rec.append(createSqlField(QStringLiteral("replyId"), newMsg.reply->id));
            rec.append(createSqlStringField(QStringLiteral("replyQuote"), newMsg.reply->quote));
        } else {
            rec.append(createNullField(QStringLiteral("replyTo")));
            rec.append(createNullField(QStringLiteral("replyId")));
            rec.append(createNullField(QStringLiteral("replyQuote")));
        }
    }
    if (oldMsg.timestamp != newMsg.timestamp) {
        rec.append(createSqlField(QStringLiteral("timestamp"), newMsg.timestamp.toString(Qt::ISODateWithMs)));
    }
    if (oldMsg.body() != newMsg.body()) {
        rec.append(createSqlField(QStringLiteral("body"), newMsg.body()));
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
        rec.append(createSqlStringField(QStringLiteral("spoilerHint"), newMsg.spoilerHint));
    }
    if (oldMsg.fileGroupId != newMsg.fileGroupId) {
        rec.append(createSqlField(QStringLiteral("fileGroupId"), optionalToVariant(newMsg.fileGroupId)));
    }
    if (const auto groupChatInvitation = newMsg.groupChatInvitation; oldMsg.groupChatInvitation != groupChatInvitation) {
        if (groupChatInvitation) {
            rec.append(createSqlStringField(QStringLiteral("groupChatInviterJid"), groupChatInvitation->inviterJid));
            rec.append(createSqlStringField(QStringLiteral("groupChatInviteeJid"), groupChatInvitation->inviterJid));
            rec.append(createSqlField(QStringLiteral("groupChatInvitationJid"), groupChatInvitation->groupChatJid));
            rec.append(createSqlStringField(QStringLiteral("groupChatToken"), groupChatInvitation->token));
        } else {
            rec.append(createNullField(QStringLiteral("groupChatInviterJid")));
            rec.append(createNullField(QStringLiteral("groupChatInviteeJid")));
            rec.append(createNullField(QStringLiteral("groupChatInvitationJid")));
            rec.append(createNullField(QStringLiteral("groupChatToken")));
        }
    }
    if (oldMsg.errorText != newMsg.errorText) {
        rec.append(createSqlStringField(QStringLiteral("errorText"), newMsg.errorText));
    }
    if (oldMsg.removed != newMsg.removed) {
        rec.append(createSqlField(QStringLiteral("removed"), newMsg.removed));
    }
    if (oldMsg.marked != newMsg.marked) {
        rec.append(createSqlField(QStringLiteral("marked"), newMsg.marked));
    }

    return rec;
}

QFuture<std::optional<Message>> MessageDb::fetchMessage(const QString &accountJid, const QString &chatJid, const QString &messageId)
{
    return run([this, accountJid, chatJid, messageId]() -> std::optional<Message> {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT *
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid AND
                                        (stanzaId = :messageId OR originId = :messageId OR id = :messageId)
                                    ORDER BY timestamp DESC
                                    LIMIT 1
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":messageId", messageId},
                  });

        auto messages = _fetchMessagesFromQuery(query);
        _fetchAdditionalData(messages);

        if (messages.isEmpty()) {
            return std::nullopt;
        }

        return messages.constFirst();
    });
}

QFuture<QList<Message>> MessageDb::fetchMessages(const QString &accountJid, const QString &chatJid, int index)
{
    return run([this, accountJid, chatJid, index]() {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT *
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid
                                    ORDER BY timestamp DESC
                                    LIMIT :index, :limit
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      {u":limit", DB_QUERY_LIMIT_MESSAGES},
                  });

        auto messages = _fetchMessagesFromQuery(query);
        _fetchAdditionalData(messages);

        return messages;
    });
}

QFuture<QList<File>> MessageDb::fetchFiles(const QString &accountJid)
{
    return run([this, accountJid]() {
        return _fetchFiles(accountJid);
    });
}

QFuture<QList<File>> MessageDb::fetchFiles(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        return _fetchFiles(accountJid, chatJid);
    });
}

QFuture<QList<File>> MessageDb::fetchDownloadedFiles(const QString &accountJid)
{
    return run([this, accountJid]() {
        auto files = _fetchFiles(accountJid);
        _extractDownloadedFiles(files);

        return files;
    });
}

QFuture<QList<File>> MessageDb::fetchDownloadedFiles(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        auto files = _fetchFiles(accountJid, chatJid);
        _extractDownloadedFiles(files);

        return files;
    });
}

QFuture<QList<MessageDb::DownloadableFile>> MessageDb::fetchAutomaticallyDownloadableFiles(const QString &accountJid)
{
    return run([this, accountJid]() {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT m.chatJid, m.id, f.*
                                    FROM files AS f
                                    INNER JOIN chatMessages AS m ON m.fileGroupId = f.fileGroupId
                                    INNER JOIN roster AS r ON r.accountJid = m.accountJid AND r.jid = m.chatJid
                                    INNER JOIN accounts AS a ON a.jid = m.accountJid
                                    WHERE m.accountJid = :accountJid AND f.transferState = 0 AND f.localFilePath IS NULL
                                )"),
                  {
                      {u":accountJid", accountJid},
                  });

        enum {
            ChatJid,
            MessageId,
            Id,
            FileGroupId,
            Name,
            Description,
            MimeType,
            Size,
            LastModified,
            Disposition,
            Thumbnail,
            LocalFilePath,
            ExternalId,
            TransferOutgoing,
            TransferState,
        };

        QList<DownloadableFile> files;
        reserve(files, query);

        while (query.next()) {
            const auto chatJid = query.value(ChatJid).toString();
            const auto messageId = query.value(MessageId).toString();
            const auto id = query.value(Id).toLongLong();

            File file;
            file.id = id;
            file.fileGroupId = query.value(FileGroupId).toLongLong();
            file.name = variantToOptional<QString>(query.value(Name));
            file.description = variantToOptional<QString>(query.value(Description));
            file.mimeType = QMimeDatabase().mimeTypeForName(query.value(MimeType).toString());
            file.size = variantToOptional<long long>(query.value(Size));
            file.lastModified = parseDateTime(query, LastModified);
            file.disposition = query.value(Disposition).value<QXmppFileShare::Disposition>();
            file.localFilePath = query.value(LocalFilePath).toString();
            file.externalId = query.value(ExternalId).toString();
            file.transferOutgoing = query.value(TransferOutgoing).toBool();
            file.transferState = query.value(TransferState).value<File::TransferState>();
            file.hashes = _fetchFileHashes(id);
            file.thumbnail = query.value(Thumbnail).toByteArray();
            file.httpSources = _fetchHttpSource(id);
            file.encryptedSources = _fetchEncryptedSource(id);

            files.append({chatJid, messageId, file});
        }

        return files;
    });
}

QFuture<QList<Message>> MessageDb::fetchMessagesUntilFirstContactMessage(const QString &accountJid, const QString &chatJid, int index)
{
    return run([this, accountJid, chatJid, index]() {
        auto query = createQuery();
        execQuery(query,
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
                                                    WHERE isOwn = 0
                                            )
                                        )
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      {u":limit", DB_QUERY_LIMIT_MESSAGES},
                  });

        auto messages = _fetchMessagesFromQuery(query);
        _fetchAdditionalData(messages);

        return messages;
    });
}

QFuture<MessageDb::MessageResult>
MessageDb::fetchMessagesUntilId(const QString &accountJid, const QString &chatJid, int index, const QString &limitingId, bool fetchMessageMinimum)
{
    return run([this, accountJid, chatJid, index, fetchMessageMinimum, limitingId]() -> MessageResult {
        auto query = createQuery();

        execQuery(query,
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
                                                (id = :limitingId OR stanzaId = :limitingId) AND
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
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      {u":limitingId", limitingId},
                  });

        query.first();
        const auto foundMessageIndex = query.value(0).toInt();
        const auto messagesUntilFoundMessageCount = std::max(0, foundMessageIndex - index);

        // Skip further processing if no message with limitingId could be found.
        if (!messagesUntilFoundMessageCount && !fetchMessageMinimum) {
            return {};
        }

        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT *
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid
                                    ORDER BY timestamp DESC
                                    LIMIT :index, :limit
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      {u":limit", messagesUntilFoundMessageCount ? messagesUntilFoundMessageCount + DB_QUERY_LIMIT_MESSAGES * 2 : DB_QUERY_LIMIT_MESSAGES},
                  });

        MessageResult result{_fetchMessagesFromQuery(query),
                             // Database index starts at 1, but message model index starts at 0.
                             foundMessageIndex - 1};

        _fetchAdditionalData(result.messages);

        return result;
    });
}

QFuture<MessageDb::MessageResult>
MessageDb::fetchMessagesUntilQueryString(const QString &accountJid, const QString &chatJid, int index, const QString &queryString)
{
    return run([this, accountJid, chatJid, index, queryString]() -> MessageResult {
        auto query = createQuery();

        execQuery(query,
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
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      // '%' is intended here as a placeholder inside the query for SQL statement "LIKE".
                      {u":queryString", QString(QLatin1Char('%') + queryString + QLatin1Char('%'))},
                  });

        query.first();
        const auto queryStringMessageIndex = query.value(0).toInt();
        const auto messagesUntilQueryStringCount = queryStringMessageIndex - index;

        // Skip further processing if no message with queryString could be found.
        if (messagesUntilQueryStringCount <= 0) {
            return {};
        }

        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT *
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid
                                    ORDER BY timestamp DESC
                                    LIMIT :index, :limit
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                      {u":limit", messagesUntilQueryStringCount + DB_QUERY_LIMIT_MESSAGES},
                  });

        MessageResult result{_fetchMessagesFromQuery(query),
                             // Database index starts at 1, but message model index starts at 0.
                             queryStringMessageIndex - 1};

        _fetchAdditionalData(result.messages);

        return result;
    });
}

Message MessageDb::_fetchLastMessage(const QString &accountJid, const QString &chatJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT *
                                FROM messages
                                WHERE accountJid = :accountJid AND chatJid = :chatJid AND removed = 0
                                ORDER BY timestamp DESC
                                LIMIT 1
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
              });

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
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT id
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid AND isOwn = 0
                                    ORDER BY timestamp DESC
                                    LIMIT :index, 1
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":index", index},
                  });

        if (query.first()) {
            return query.value(0).toString();
        }

        return QString();
    });
}

bool MessageDb::_hasMessage(const QString &accountJid, const QString &chatJid, const QString &groupChatSenderId)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT 1
                                FROM chatMessages
                                WHERE accountJid = :accountJid AND chatJid = :chatJid AND groupChatSenderId = :groupChatSenderId
                                LIMIT 1
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
                  {u":groupChatSenderId", groupChatSenderId},
              });

    return query.first();
}

QFuture<int> MessageDb::latestContactMessageCount(const QString &accountJid, const QString &chatJid, const QString &messageIdBegin)
{
    return run([this, accountJid, chatJid, messageIdBegin] {
        return _latestContactMessageCount(accountJid, chatJid, messageIdBegin);
    });
}

int MessageDb::_latestContactMessageCount(const QString &accountJid, const QString &chatJid, const QString &messageIdBegin)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT COUNT(*)
                                FROM chatMessages
                                WHERE
                                    accountJid = :accountJid AND chatJid = :chatJid AND isOwn = 0 AND
                                    (
                                        :messageIdBegin IS NULL OR
                                        (
                                            timestamp >
                                            (
                                                SELECT timestamp
                                                FROM chatMessages
                                                WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageIdBegin
                                                LIMIT 1
                                            )
                                        )
                                    )
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
                  {u":messageIdBegin", messageIdBegin},
              });

    query.first();
    return query.value(0).toInt();
}

QFuture<int> MessageDb::markedMessageCount(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid] {
        return _markedMessageCount(accountJid, chatJid);
    });
}

int MessageDb::_markedMessageCount(const QString &accountJid, const QString &chatJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT COUNT(*)
                                FROM chatMessages
                                WHERE
                                    accountJid = :accountJid AND chatJid = :chatJid AND marked = 1
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
              });

    query.first();
    return query.value(0).toInt();
}

bool MessageDb::_checkMoreRecentMessageExists(const QString &accountJid, const QString &chatJid, const QDateTime &timestamp, int offset)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT COUNT(*)
                                FROM chatMessages
                                WHERE
                                    accountJid = :accountJid AND chatJid = :chatJid AND
                                    timestamp >= :timestamp
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
                  {u":timestamp", timestamp.toString(Qt::ISODateWithMs)},
              });

    query.first();
    return query.value(0).toInt() > offset;
}

QFuture<void> MessageDb::addMessage(const Message &message, MessageOrigin origin)
{
    Q_ASSERT(message.deliveryState != DeliveryState::Draft);

    return run([this, message, origin]() {
        _addMessage(message, origin);
    });
}

void MessageDb::_addMessage(Message message, MessageOrigin origin)
{
    _fetchGroupChatUser(message);
    _fetchTrustLevel(message);
    _fetchReply(message);

    Q_EMIT messageAdded(message, origin);

    _addMessage(message);
    _setFiles(message.files);
}

QFuture<void> MessageDb::addOrUpdateMessage(const Message &message, MessageOrigin origin, const std::function<void(Message &)> &updateMessage)
{
    Q_ASSERT(message.deliveryState != DeliveryState::Draft);

    return run([this, message, origin, updateMessage]() {
        if (_checkMessageExists(message)) {
            _updateMessage(message.accountJid, message.chatJid, message.originId, updateMessage);
        } else {
            _addMessage(message, origin);
        }
    });
}

QFuture<void>
MessageDb::updateMessage(const QString &accountJid, const QString &chatJid, const QString &messageId, const std::function<void(Message &)> &updateMsg)
{
    return run([this, accountJid, chatJid, messageId, updateMsg]() {
        _updateMessage(accountJid, chatJid, messageId, updateMsg);
    });
}

QFuture<void> MessageDb::removeMessages(const QString &accountJid)
{
    return run([this, accountJid]() {
        _removeReactions(accountJid);
        _removeFiles(accountJid);

        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    DELETE FROM messages WHERE accountJid = :accountJid
                                )"),
                  {
                      {u":accountJid", accountJid},
                  });
    });
}

QFuture<void> MessageDb::removeMessages(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        _removeReactions(accountJid);
        _removeFiles(accountJid, chatJid);

        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    DELETE FROM messages WHERE accountJid = :accountJid AND chatJid = :chatJid
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                  });

        messagesRemoved(accountJid, chatJid);
    });
}

QFuture<void> MessageDb::removeMessage(const QString &accountJid, const QString &chatJid, const QString &messageId)
{
    return run([this, accountJid, chatJid, messageId]() {
        auto query = createQuery();

        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT id
                                    FROM chatMessages
                                    WHERE
                                        accountJid = :accountJid AND chatJid = :chatJid AND
                                        (id = :messageId OR stanzaId = :messageId OR replaceId = :messageId)
                                    LIMIT 1
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":messageId", messageId},
                  });

        if (query.next()) {
            const auto foundMessageId = query.value(0).toString();

            _removeReactions(accountJid, chatJid, messageId);
            _removeFiles(accountJid, chatJid, foundMessageId);

            // Set the message's content to NULL and the "removed" flag to true.
            execQuery(query,
                      QStringLiteral(R"(
                                        UPDATE messages
                                        SET
                                            replyTo = NULL,
                                            replyId = NULL,
                                            replyQuote = NULL,
                                            body = NULL,
                                            spoilerHint = NULL,
                                            fileGroupId = NULL,
                                            groupChatInviterJid = NULL,
                                            groupChatInviteeJid = NULL,
                                            groupChatInvitationJid = NULL,
                                            groupChatToken = NULL,
                                            errorText = NULL,
                                            removed = 1
                                        WHERE
                                            accountJid = :accountJid AND chatJid = :chatJid AND id = :messageId
                                    )"),
                      {
                          {u":accountJid", accountJid},
                          {u":chatJid", chatJid},
                          {u":messageId", foundMessageId},
                      });
        }

        Q_EMIT messageRemoved(_initializeLastMessage(accountJid, chatJid));
    });
}

QFuture<void> MessageDb::attachFileSources(const QString &accountJid,
                                           const QString &chatJid,
                                           const QString &messageId,
                                           const QString &externalFileId,
                                           const QList<HttpSource> &httpSources,
                                           const QList<EncryptedSource> &encryptedSources)
{
    return run([=, this] {
        // fetch message
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral("SELECT * FROM chatMessages WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :id"),
                  {{u":accountJid", accountJid}, {u":chatJid", chatJid}, {u":id", messageId}});
        auto msgs = _fetchMessagesFromQuery(query);
        if (msgs.empty()) {
            qCDebug(KAIDAN_CORE_LOG) << "Could not find message with ID" << messageId << "to attach file sources.";
            return;
        }
        _fetchAdditionalData(msgs);

        auto message = msgs.takeFirst();

        // find file with external ID
        auto fileItr = std::ranges::find(message.files, externalFileId, &File::externalId);
        if (fileItr == message.files.end()) {
            qCDebug(KAIDAN_CORE_LOG) << "Could not attach sources to message with ID" << messageId << ": No file with external ID of" << externalFileId
                                     << "found.";
            return;
        }
        auto &file = *fileItr;

        // append new sources and set correct database file ID
        std::ranges::transform(httpSources, std::back_inserter(file.httpSources), [&](auto s) {
            s.fileId = file.id;
            return s;
        });
        std::ranges::transform(encryptedSources, std::back_inserter(file.encryptedSources), [&](auto s) {
            s.fileId = file.id;
            return s;
        });

        Q_EMIT messageUpdated(message);

        // add sources to DB
        if (!httpSources.empty()) {
            _setHttpSources(file.httpSources);
        }
        if (!encryptedSources.empty()) {
            _setEncryptedSources(file.encryptedSources);
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

        _addMessage(msg);
        Q_EMIT draftMessageAdded(msg);
    });
}

QFuture<void> MessageDb::updateDraftMessage(const QString &accountJid, const QString &chatJid, const std::function<void(Message &)> &updateMessage)
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
                execQuery(query,
                          driver.sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_MESSAGES), rec, false)
                              + simpleWhereStatement(&driver,
                                                     {
                                                         {QStringLiteral("accountJid"), newMessage.accountJid},
                                                         {QStringLiteral("chatJid"), newMessage.chatJid},
                                                         {QStringLiteral("deliveryState"), static_cast<int>(newMessage.deliveryState)},
                                                     }));

                Q_EMIT draftMessageUpdated(newMessage);
            }
        }
    });
}

QFuture<void> MessageDb::removeDraftMessage(const QString &accountJid, const QString &chatJid)
{
    return run([this, accountJid, chatJid]() {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    DELETE FROM messages
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid AND deliveryState = :deliveryState
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":chatJid", chatJid},
                      {u":deliveryState", int(DeliveryState::Draft)},
                  });

        Q_EMIT draftMessageRemoved(_initializeLastMessage(accountJid, chatJid));
    });
}

void MessageDb::_addMessage(const Message &message)
{
    SqlUtils::QueryBindValues values = {
        {u"accountJid", message.accountJid},
        {u"chatJid", message.chatJid},
        {u"isOwn", message.isOwn},
        {u"groupChatSenderId", message.groupChatSenderId},
        {u"id", message.id},
        {u"originId", message.originId},
        {u"stanzaId", message.stanzaId},
        {u"replaceId", message.replaceId},
        {u"timestamp", message.timestamp.toString(Qt::ISODateWithMs)},
        {u"body", message.body()},
        {u"encryption", message.encryption},
        {u"senderKey", message.senderKey},
        {u"deliveryState", int(message.deliveryState)},
        {u"isSpoiler", message.isSpoiler},
        {u"spoilerHint", message.spoilerHint},
        {u"fileGroupId", optionalToVariant(message.fileGroupId)},
        {u"errorText", message.errorText},
        {u"marked", message.marked},
        {u"removed", message.removed},
    };

    if (const auto reply = message.reply) {
        values.insert({
            {u"replyTo", message.isGroupChatMessage() ? reply->toGroupChatParticipantId : reply->toJid},
            {u"replyId", reply->id},
            {u"replyQuote", reply->quote},
        });
    }

    if (const auto groupChatInvitation = message.groupChatInvitation) {
        values.insert({
            {u"groupChatInviterJid", groupChatInvitation->inviterJid},
            {u"groupChatInviteeJid", groupChatInvitation->inviteeJid},
            {u"groupChatInvitationJid", groupChatInvitation->groupChatJid},
            {u"groupChatToken", groupChatInvitation->token},
        });
    }

    // "DatabaseComponent::insert()" cannot be used here because the binary data of
    // "msg.senderKey()" is not appropriately inserted into the database with QSqlField.
    insertBinary(QString::fromLatin1(DB_TABLE_MESSAGES), values);
}

void MessageDb::_updateMessage(const QString &accountJid, const QString &chatJid, const QString &messageId, const std::function<void(Message &)> &updateMsg)
{
    // load current message item from db
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT *
                                FROM chatMessages
                                WHERE
                                    accountJid = :accountJid AND
                                    chatJid = :chatJid AND
                                    (replaceId = :id OR stanzaId = :id OR originId = :id OR id = :id)
                                LIMIT 1
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
                  {u":id", messageId},
              });

    auto msgs = _fetchMessagesFromQuery(query);
    _fetchAdditionalData(msgs);

    // update loaded item
    if (!msgs.isEmpty()) {
        const auto &oldMessage = msgs.first();
        Q_ASSERT(oldMessage.deliveryState != DeliveryState::Draft);

        Message newMessage = oldMessage;
        updateMsg(newMessage);
        Q_ASSERT(newMessage.deliveryState != DeliveryState::Draft);

        _fetchReply(newMessage);

        // Replace the old message's values with the updated ones if the message has changed.
        if (oldMessage != newMessage) {
            Q_EMIT messageUpdated(newMessage);

            const auto &oldReactionSenders = oldMessage.reactionSenders;
            if (const auto &newReactionSenders = newMessage.reactionSenders; oldReactionSenders != newReactionSenders) {
                // Remove old reactions.
                for (auto itr = oldReactionSenders.begin(); itr != oldReactionSenders.end(); ++itr) {
                    const auto &senderId = itr.key();
                    const auto &reactionSender = itr.value();

                    for (const auto &reaction : reactionSender.reactions) {
                        if (!newReactionSenders.value(senderId).reactions.contains(reaction)) {
                            if (senderId == oldMessage.accountJid) {
                                execQuery(query,
                                          QStringLiteral(R"(
                                                            DELETE FROM messageReactions
                                                            WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId AND senderId IS NULL AND emoji = :emoji
                                                        )"),
                                          {
                                              {u":accountJid", oldMessage.accountJid},
                                              {u":chatJid", oldMessage.chatJid},
                                              {u":messageSenderId",
                                               oldMessage.isOwn ? oldMessage.accountJid
                                                                : (oldMessage.isGroupChatMessage() ? oldMessage.groupChatSenderId : oldMessage.chatJid)},
                                              {u":messageId", oldMessage.relevantId()},
                                              {u":emoji", reaction.emoji},
                                          });
                            } else {
                                execQuery(query,
                                          QStringLiteral(R"(
                                                            DELETE FROM messageReactions
                                                            WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId AND senderId = :senderId AND emoji = :emoji
                                                        )"),
                                          {
                                              {u":accountJid", oldMessage.accountJid},
                                              {u":chatJid", oldMessage.chatJid},
                                              {u":messageSenderId",
                                               oldMessage.isOwn ? oldMessage.accountJid
                                                                : (oldMessage.isGroupChatMessage() ? oldMessage.groupChatSenderId : oldMessage.chatJid)},
                                              {u":messageId", oldMessage.relevantId()},
                                              {u":senderId", senderId},
                                              {u":emoji", reaction.emoji},
                                          });
                            }
                        }
                    }
                }

                // Add new reactions.
                for (auto itr = newReactionSenders.begin(); itr != newReactionSenders.end(); ++itr) {
                    const auto &senderId = itr.key();
                    const auto &reactionSender = itr.value();

                    for (const auto &reaction : reactionSender.reactions) {
                        if (!oldReactionSenders.value(senderId).reactions.contains(reaction)) {
                            insert(QString::fromLatin1(DB_TABLE_MESSAGE_REACTIONS),
                                   {
                                       {u"accountJid", oldMessage.accountJid},
                                       {u"chatJid", oldMessage.chatJid},
                                       {u"messageSenderId",
                                        oldMessage.isOwn ? oldMessage.accountJid
                                                         : (oldMessage.isGroupChatMessage() ? oldMessage.groupChatSenderId : oldMessage.chatJid)},
                                       {u"messageId", oldMessage.relevantId()},
                                       {u"senderId", senderId == oldMessage.accountJid ? QVariant{} : senderId},
                                       {u"timestamp", reactionSender.latestTimestamp},
                                       {u"deliveryState", static_cast<int>(reaction.deliveryState)},
                                       {u"emoji", reaction.emoji},
                                   });
                        }
                    }
                }
            } else if (auto rec = createUpdateRecord(oldMessage, newMessage); !rec.isEmpty()) {
                auto &driver = sqlDriver();

                // Create an SQL record containing only the differences.
                execQuery(query,
                          driver.sqlStatement(QSqlDriver::UpdateStatement, QStringLiteral(DB_TABLE_MESSAGES), rec, false)
                              + simpleWhereStatement(&driver,
                                                     {
                                                         {QStringLiteral("accountJid"), oldMessage.accountJid},
                                                         {QStringLiteral("chatJid"), oldMessage.chatJid},
                                                         {QStringLiteral("id"), oldMessage.id},
                                                     }));
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

            // add new files, replace changed files
            _setFiles(newMessage.files);
        }
    }
}

void MessageDb::fetchLatestFileData()
{
    run([this]() {
        _fetchLatestFileId();
        _fetchLatestFileGroupId();
    });
}

void MessageDb::_fetchLatestFileId()
{
    enum {
        Id,
    };

    auto query = createQuery();
    execQuery(query, QStringLiteral(R"(
                                       SELECT id
                                       FROM files
                                       ORDER BY id DESC
                                       LIMIT 1
                                   )"));

    if (query.first()) {
        m_latestFileId = query.value(Id).toLongLong();
    }
}

void MessageDb::_fetchLatestFileGroupId()
{
    enum {
        FileGroupId,
    };

    auto query = createQuery();
    execQuery(query, QStringLiteral(R"(
                                       SELECT fileGroupId
                                       FROM files
                                       ORDER BY fileGroupId DESC
                                       LIMIT 1
                                   )"));

    if (query.first()) {
        m_latestFileGroupId = query.value(FileGroupId).toLongLong();
    }
}

void MessageDb::_setFiles(const QList<File> &files)
{
    thread_local static auto query = [this]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral(R"(
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
                                                  localFilePath,
                                                  externalId,
                                                  transferOutgoing,
                                                  transferState
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
                                                  :localFilePath,
                                                  :externalId,
                                                  :transferOutgoing,
                                                  :transferState
                                              )
                                          )"));
        return query;
    }();

    for (const auto &file : files) {
        bindValues(query,
                   {
                       {u":id", file.id},
                       {u":fileGroupId", file.fileGroupId},
                       {u":name", optionalToVariant(file.name)},
                       {u":description", optionalToVariant(file.description)},
                       {u":mimeType", file.mimeType.name()},
                       {u":size", optionalToVariant(file.size)},
                       {u":lastModified", serialize(file.lastModified)},
                       {u":disposition", int(file.disposition)},
                       {u":thumbnail", file.thumbnail},
                       {u":localFilePath", file.localFilePath},
                       {u":externalId", file.externalId},
                       {u":transferOutgoing", file.transferOutgoing},
                       {u":transferState", Enums::toIntegral(file.transferState)},
                   });
        execQuery(query);

        _setFileHashes(file.hashes);
        _setHttpSources(file.httpSources);
        _setEncryptedSources(file.encryptedSources);
    }
}

void MessageDb::_setFileHashes(const QList<FileHash> &fileHashes)
{
    thread_local static auto query = [this]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral(R"(
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
                                          )"));
        return query;
    }();

    for (const auto &hash : fileHashes) {
        bindValues(query,
                   {
                       {u":dataId", hash.dataId},
                       {u":hashType", int(hash.hashType)},
                       {u":hashValue", hash.hashValue},
                   });
        execQuery(query);
    }
}

void MessageDb::_setHttpSources(const QList<HttpSource> &sources)
{
    thread_local static auto query = [this]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral(R"(
                                              INSERT OR REPLACE INTO fileHttpSources (
                                                  fileId,
                                                  url
                                              )
                                              VALUES (
                                                  :fileId,
                                                  :url
                                              )
                                          )"));
        return query;
    }();

    for (const auto &source : sources) {
        bindValues(query,
                   {
                       {u":fileId", source.fileId},
                       {u":url", source.url.toEncoded()},
                   });
        execQuery(query);
    }
}

void MessageDb::_setEncryptedSources(const QList<EncryptedSource> &sources)
{
    thread_local static auto query = [this]() {
        auto query = createQuery();
        prepareQuery(query, QStringLiteral(R"(
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
                                          )"));
        return query;
    }();

    for (const auto &source : sources) {
        bindValues(query,
                   {
                       {u":fileId", source.fileId},
                       {u":url", source.url.toEncoded()},
                       {u":cipher", int(source.cipher)},
                       {u":key", source.key},
                       {u":iv", source.iv},
                       {u":encryptedDataId", optionalToVariant(source.encryptedDataId)},
                   });
        execQuery(query);

        _setFileHashes(source.encryptedHashes);
    }
}

void MessageDb::_removeFiles(const QString &accountJid)
{
    _removeFiles(QStringLiteral(R"(
                                   SELECT fileGroupId
                                   FROM messages
                                   WHERE accountJid = :accountJid
                                )"),
                 {
                     {u":accountJid", accountJid},
                 });
}

void MessageDb::_removeFiles(const QString &accountJid, const QString &chatJid)
{
    _removeFiles(QStringLiteral(R"(
                                   SELECT fileGroupId
                                   FROM messages
                                   WHERE accountJid = :accountJid AND chatJid = :chatJid
                                )"),
                 {
                     {u":accountJid", accountJid},
                     {u":chatJid", chatJid},
                 });
}

void MessageDb::_removeFiles(const QString &accountJid, const QString &chatJid, const QString &messageId)
{
    _removeFiles(QStringLiteral(R"(
                                   SELECT fileGroupId
                                   FROM messages
                                   WHERE accountJid = :accountJid AND chatJid = :chatJid AND id = :messageId
                                )"),
                 {
                     {u":accountJid", accountJid},
                     {u":chatJid", chatJid},
                     {u":messageId", messageId},
                 });
}

void MessageDb::_removeFiles(const QList<qint64> &fileIds)
{
    _removeFileHashes(fileIds);
    _removeHttpSources(fileIds);
    _removeEncryptedSources(fileIds);

    auto query = createQuery();
    prepareQuery(query, QStringLiteral("DELETE FROM files WHERE id = :fileId"));
    for (auto fileId : fileIds) {
        bindValues(query, {{u":fileId", fileId}});
        execQuery(query);
    }
}

void MessageDb::_removeFiles(const QString &statement, const QueryBindValues &bindValues)
{
    auto query = createQuery();
    execQuery(query, statement, bindValues);

    while (query.next()) {
        const auto fileGroupId = query.value(0).toLongLong();

        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                SELECT id
                                FROM files
                                WHERE fileGroupId = :fileGroupId
                            )"),
                  {
                      {u":fileGroupId", fileGroupId},
                  });

        QList<qint64> fileIds;
        reserve(fileIds, query);

        while (query.next()) {
            fileIds.append(query.value(0).toLongLong());
        }

        if (!fileIds.isEmpty()) {
            _removeFiles(fileIds);
        }
    }
}

void MessageDb::_removeFileHashes(const QList<qint64> &fileIds)
{
    auto query = createQuery();
    prepareQuery(query, QStringLiteral("DELETE FROM fileHashes WHERE dataId = :fileId"));
    for (auto fileId : fileIds) {
        bindValues(query, {{u":fileId", fileId}});
        execQuery(query);
    }
}

void MessageDb::_removeHttpSources(const QList<qint64> &fileIds)
{
    auto query = createQuery();
    prepareQuery(query, QStringLiteral("DELETE FROM fileHttpSources WHERE fileId = :fileId"));
    for (auto fileId : fileIds) {
        bindValues(query, {{u":fileId", fileId}});
        execQuery(query);
    }
}

void MessageDb::_removeEncryptedSources(const QList<qint64> &fileIds)
{
    auto query = createQuery();
    prepareQuery(query, QStringLiteral("DELETE FROM fileEncryptedSources WHERE fileId = :fileId"));
    for (auto fileId : fileIds) {
        bindValues(query, {{u":fileId", fileId}});
        execQuery(query);
    }
}

QList<File> MessageDb::_fetchFiles(const QString &accountJid)
{
    Q_ASSERT(!accountJid.isEmpty());

    return _fetchFiles(QStringLiteral(R"(
                                         SELECT fileGroupId
                                         FROM chatMessages
                                         WHERE accountJid = :accountJid AND fileGroupId IS NOT NULL
                                     )"),
                       {
                           {u":accountJid", accountJid},
                       });
}

QList<File> MessageDb::_fetchFiles(const QString &accountJid, const QString &chatJid)
{
    Q_ASSERT(!accountJid.isEmpty() && !chatJid.isEmpty());

    return _fetchFiles(QStringLiteral(R"(
                                         SELECT fileGroupId
                                         FROM chatMessages
                                         WHERE accountJid = :accountJid AND chatJid = :chatJid AND fileGroupId IS NOT NULL
                                     )"),
                       {
                           {u":accountJid", accountJid},
                           {u":chatJid", chatJid},
                       });
}

QList<File> MessageDb::_fetchFiles(const QString &statement, const QueryBindValues &bindValues)
{
    enum {
        FileGroupId,
    };

    auto query = createQuery();
    execQuery(query, statement, bindValues);

    QList<File> files;
    reserve(files, query);

    while (query.next()) {
        files.append(_fetchFiles(query.value(FileGroupId).toLongLong()));
    }

    return files;
}

void MessageDb::_extractDownloadedFiles(QList<File> &files)
{
    files.erase(std::remove_if(files.begin(),
                               files.end(),
                               [](const File &file) {
                                   return !QFile::exists(file.localFilePath);
                               }),
                files.end());
}

QList<File> MessageDb::_fetchFiles(qint64 fileGroupId)
{
    enum {
        Id,
        Name,
        Description,
        MimeType,
        Size,
        LastModified,
        Disposition,
        Thumbnail,
        LocalFilePath,
        ExternalId,
        TransferOutgoing,
        TransferState,
    };

    thread_local static auto query = [this]() {
        auto q = createQuery();
        prepareQuery(q,
                     QStringLiteral("SELECT id, name, description, mimeType, size, lastModified, disposition, "
                                    "thumbnail, localFilePath, externalId, transferOutgoing, transferState FROM files "
                                    "WHERE fileGroupId = :fileGroupId"));
        return q;
    }();

    bindValues(query, {{u":fileGroupId", QVariant(fileGroupId)}});
    execQuery(query);

    QList<File> files;
    reserve(files, query);
    while (query.next()) {
        const auto id = query.value(Id).toLongLong();

        File file;
        file.id = id;
        file.fileGroupId = fileGroupId;
        file.name = variantToOptional<QString>(query.value(Name));
        file.description = variantToOptional<QString>(query.value(Description));
        file.mimeType = QMimeDatabase().mimeTypeForName(query.value(MimeType).toString());
        file.size = variantToOptional<long long>(query.value(Size));
        file.lastModified = parseDateTime(query, LastModified);
        file.disposition = query.value(Disposition).value<QXmppFileShare::Disposition>();
        file.localFilePath = query.value(LocalFilePath).toString();
        file.externalId = query.value(ExternalId).toString();
        file.transferOutgoing = query.value(TransferOutgoing).toBool();
        file.transferState = query.value(TransferState).value<File::TransferState>();
        file.hashes = _fetchFileHashes(id);
        file.thumbnail = query.value(Thumbnail).toByteArray();
        file.httpSources = _fetchHttpSource(id);
        file.encryptedSources = _fetchEncryptedSource(id);

        files.append(file);
    }
    return files;
}

QList<FileHash> MessageDb::_fetchFileHashes(qint64 fileId)
{
    enum {
        HashType,
        HashValue,
    };

    thread_local static auto query = [this]() {
        auto q = createQuery();
        prepareQuery(q, QStringLiteral("SELECT hashType, hashValue FROM fileHashes WHERE dataId = :fileId"));
        return q;
    }();

    bindValues(query, {{u":fileId", fileId}});
    execQuery(query);

    QList<FileHash> hashes;
    reserve(hashes, query);
    while (query.next()) {
        hashes << FileHash{fileId, query.value(HashType).value<QXmpp::HashAlgorithm>(), query.value(HashValue).toByteArray()};
    }
    return hashes;
}

QList<HttpSource> MessageDb::_fetchHttpSource(qint64 fileId)
{
    enum {
        Url,
    };

    thread_local static auto query = [this]() {
        auto q = createQuery();
        prepareQuery(q, QStringLiteral("SELECT url FROM fileHttpSources WHERE fileId = :fileId"));
        return q;
    }();

    bindValues(query, {{u":fileId", fileId}});
    execQuery(query);

    QList<HttpSource> sources;
    reserve(sources, query);
    while (query.next()) {
        sources << HttpSource{fileId, QUrl::fromEncoded(query.value(Url).toByteArray())};
    }
    return sources;
}

QList<EncryptedSource> MessageDb::_fetchEncryptedSource(qint64 fileId)
{
    enum {
        Url,
        Cipher,
        Key,
        Iv,
        EncryptedDataId,
    };

    thread_local static auto query = [this]() {
        auto q = createQuery();
        prepareQuery(q,
                     QStringLiteral("SELECT url, cipher, key, iv, encryptedDataId FROM fileEncryptedSources "
                                    "WHERE fileId = :fileId"));
        return q;
    }();

    bindValues(query, {{u":fileId", fileId}});
    execQuery(query);

    auto parseHashes = [this](QSqlQuery &query) -> QList<FileHash> {
        auto dataId = query.value(EncryptedDataId);
        if (dataId.isNull()) {
            return {};
        }
        return _fetchFileHashes(dataId.toLongLong());
    };

    QList<EncryptedSource> sources;
    reserve(sources, query);
    while (query.next()) {
        sources << EncryptedSource{
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

void MessageDb::_fetchAdditionalData(QList<Message> &messages)
{
    _fetchReactions(messages);
    _fetchGroupChatUsers(messages);
    _fetchTrustLevels(messages);
}

void MessageDb::_fetchReactions(QList<Message> &messages)
{
    enum {
        SenderId,
        Emoji,
        Timestamp,
        DeliveryState,
    };

    auto query = createQuery();

    for (auto &message : messages) {
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT senderId, emoji, timestamp, deliveryState
                                    FROM messageReactions
                                    WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId
                                )"),
                  {
                      {u":accountJid", message.accountJid},
                      {u":chatJid", message.chatJid},
                      {u":messageSenderId", message.isOwn ? message.accountJid : (message.isGroupChatMessage() ? message.groupChatSenderId : message.chatJid)},
                      {u":messageId", message.relevantId()},
                  });

        // Iterate over all found emojis.
        while (query.next()) {
            MessageReaction reaction;
            reaction.emoji = query.value(Emoji).toString();
            reaction.deliveryState = query.value(DeliveryState).value<MessageReactionDeliveryState::Enum>();

            const auto senderId = query.value(SenderId).toString();
            auto &reactionSender = message.reactionSenders[senderId.isEmpty() ? message.accountJid : senderId];

            // Use the timestamp of the current emoji as the latest timestamp if the emoji's
            // timestamp is newer than the latest one.
            if (const auto timestamp = query.value(Timestamp).toDateTime(); reactionSender.latestTimestamp < timestamp) {
                reactionSender.latestTimestamp = timestamp;
            }

            reactionSender.reactions.append(reaction);
        }
    }
}

void MessageDb::_removeReactions(const QString &accountJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                DELETE FROM messageReactions
                                WHERE accountJid = :accountJid
                            )"),
              {
                  {u":accountJid", accountJid},
              });
}

void MessageDb::_removeReactions(const QString &accountJid, const QString &chatJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                DELETE FROM messageReactions
                                WHERE accountJid = :accountJid AND chatJid = :chatJid
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
              });
}

void MessageDb::_removeReactions(const QString &accountJid, const QString &chatJid, const QString &messageId)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                DELETE FROM messageReactions
                                WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageId = :messageId
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
                  {u":messageId", messageId},
              });
}

void MessageDb::_fetchGroupChatUsers(QList<Message> &messages)
{
    for (auto &message : messages) {
        _fetchGroupChatUser(message);
    }
}

void MessageDb::_fetchGroupChatUser(Message &message)
{
    if (message.isGroupChatMessage()) {
        if (const auto groupChatUser = GroupChatUserDb::instance()->_user(message.accountJid, message.chatJid, message.groupChatSenderId)) {
            message.groupChatSenderJid = groupChatUser->jid;
            message.groupChatSenderName = groupChatUser->displayName();
        }
    }
}

void MessageDb::_fetchTrustLevels(QList<Message> &messages)
{
    for (auto &message : messages) {
        _fetchTrustLevel(message);
    }
}

void MessageDb::_fetchTrustLevel(Message &message)
{
    if (message.encryption == Encryption::Omemo2) {
        if (const auto senderKey = message.senderKey; senderKey.isEmpty()) {
            // The message is sent from this device.
            message.preciseTrustLevel = QXmpp::TrustLevel::Authenticated;
        } else {
            message.preciseTrustLevel = TrustDb::_trustLevel(database(), message.accountJid, XMLNS_OMEMO_2, message.senderJid(), senderKey);
        }
    }
}

void MessageDb::_fetchReply(Message &message)
{
    if (auto &reply = message.reply; reply) {
        if (const auto &toGroupChatParticipantId = reply->toGroupChatParticipantId; !toGroupChatParticipantId.isEmpty()) {
            if (const auto groupChatUser = GroupChatUserDb::instance()->_user(message.accountJid, message.chatJid, toGroupChatParticipantId)) {
                reply->toJid = groupChatUser->jid;
                reply->toGroupChatParticipantName = groupChatUser->name;
            }
        }

        if (message.reply->quote.isEmpty()) {
            auto query = createQuery();
            execQuery(query,
                      QStringLiteral(R"(
                                        SELECT body
                                        FROM chatMessages
                                        WHERE accountJid = :accountJid AND chatJid = :chatJid AND
                                            (id = :id OR stanzaId = :id)
                                    )"),
                      {
                          {u":accountJid", message.accountJid},
                          {u":chatJid", message.chatJid},
                          {u":id", reply->id},
                      });

            if (query.first()) {
                message.reply->quote = query.value(0).toString();
            }
        }
    }
}

std::optional<Message> MessageDb::_fetchDraftMessage(const QString &accountJid, const QString &chatJid)
{
    auto query = createQuery();
    execQuery(query,
              QStringLiteral(R"(
                                SELECT *
                                FROM draftMessages
                                WHERE accountJid = :accountJid AND chatJid = :chatJid
                            )"),
              {
                  {u":accountJid", accountJid},
                  {u":chatJid", chatJid},
              });

    const auto messages = _fetchMessagesFromQuery(query);

    if (messages.isEmpty()) {
        return std::nullopt;
    }

    return messages.constFirst();
}

bool MessageDb::_checkMessageExists(const Message &message)
{
    QueryBindValues bindValues = {
        {u":accountJid", message.accountJid},
        {u":chatJid", message.chatJid},
    };

    // Check which IDs to check
    QStringList idChecks;

    if (!message.replaceId.isEmpty()) {
        idChecks << QStringLiteral("replaceId = :replaceId");
        bindValues.insert(u":replaceId", message.replaceId);
    }

    if (!message.stanzaId.isEmpty()) {
        idChecks << QStringLiteral("stanzaId = :stanzaId");
        bindValues.insert(u":stanzaId", message.stanzaId);
    }

    if (!message.originId.isEmpty()) {
        idChecks << QStringLiteral("originId = :originId");
        bindValues.insert(u":originId", message.originId);
    }
    if (!message.id.isEmpty()) {
        idChecks << QStringLiteral("id = :id");
        bindValues.insert(u":id", message.id);
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
    const QString querySql = QStringLiteral("SELECT COUNT(*) FROM " DB_TABLE_MESSAGES
                                            " "
                                            "WHERE (accountJid = :accountJid AND chatJid = :chatJid AND (")
        % idConditionSql % QStringLiteral(")) AND deliveryState != 4 ") % QStringLiteral("ORDER BY timestamp DESC LIMIT " CHECK_MESSAGE_EXISTS_DEPTH_LIMIT);

    auto query = createQuery();
    execQuery(query, querySql, bindValues);

    query.first();
    return query.value(0).toInt() > 0;
}

QFuture<QList<Message>> MessageDb::fetchPendingMessages(const QString &accountJid)
{
    return run([this, accountJid]() {
        auto query = createQuery();
        execQuery(query,
                  QStringLiteral(R"(
                                    SELECT *
                                    FROM chatMessages
                                    WHERE accountJid = :accountJid AND deliveryState = :deliveryState
                                    ORDER BY timestamp ASC
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":deliveryState", int(Enums::DeliveryState::Pending)},
                  });

        return _fetchMessagesFromQuery(query);
    });
}

QFuture<QMap<QString, QMap<QString, MessageReactionSender>>> MessageDb::fetchPendingReactions(const QString &accountJid)
{
    return run([this, accountJid]() {
        enum {
            ChatJid,
            MessageSenderId,
            MessageId,
        };

        auto pendingReactionQuery = createQuery();
        execQuery(pendingReactionQuery,
                  QStringLiteral(R"(
                                    SELECT DISTINCT chatJid, messageSenderId, messageId
                                    FROM messageReactions
                                    WHERE
                                        accountJid = :accountJid AND senderId IS NULL AND
                                        (deliveryState = :deliveryState1 OR deliveryState = :deliveryState2 OR deliveryState = :deliveryState3)
                                )"),
                  {
                      {u":accountJid", accountJid},
                      {u":deliveryState1", int(MessageReactionDeliveryState::PendingAddition)},
                      {u":deliveryState2", int(MessageReactionDeliveryState::PendingRemovalAfterSent)},
                      {u":deliveryState3", int(MessageReactionDeliveryState::PendingRemovalAfterDelivered)},
                  });

        // JIDs of chats mapped to IDs of messages mapped to MessageReactionSender
        QMap<QString, QMap<QString, MessageReactionSender>> reactions;

        // Iterate over all IDs of messages with pending reactions.
        while (pendingReactionQuery.next()) {
            const auto chatJid = pendingReactionQuery.value(ChatJid).toString();
            const auto messageSenderId = pendingReactionQuery.value(MessageSenderId).toString();
            const auto messageId = pendingReactionQuery.value(MessageId).toString();

            enum {
                Emoji,
                Timestamp,
                DeliveryState,
            };

            auto reactionQuery = createQuery();
            execQuery(reactionQuery,
                      QStringLiteral(R"(
                                        SELECT emoji, timestamp, deliveryState
                                        FROM messageReactions
                                        WHERE accountJid = :accountJid AND chatJid = :chatJid AND messageSenderId = :messageSenderId AND messageId = :messageId AND senderId IS NULL
                                    )"),
                      {
                          {u":accountJid", accountJid},
                          {u":chatJid", chatJid},
                          {u":messageSenderId", messageSenderId},
                          {u":messageId", messageId},
                      });

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

#include "moc_MessageDb.cpp"
