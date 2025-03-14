// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageComposition.h"

// Qt
#include <QFileDialog>
#include <QFutureWatcher>
#include <QGuiApplication>
#include <QMimeDatabase>
// KDE
#include <KFileItem>
#include <KIO/PreviewJob>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Algorithms.h"
#include "ChatController.h"
#include "FileSharingController.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "Kaidan.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "RosterModel.h"
#include "ServerFeaturesCache.h"

MessageComposition::MessageComposition()
    : m_fileSelectionModel(new FileSelectionModel(this))
{
    connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this]() {
        saveDraft();
    });
}

void MessageComposition::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        // Save the draft of the last chat when the current chat is changed.
        saveDraft();

        m_accountJid = accountJid;
        Q_EMIT accountJidChanged();

        loadDraft();
    }
}

void MessageComposition::setChatJid(const QString &chatJid)
{
    if (m_chatJid != chatJid) {
        // Save the draft of the last chat when the current chat is changed.
        saveDraft();

        m_chatJid = chatJid;
        Q_EMIT chatJidChanged();

        loadDraft();
    }
}

void MessageComposition::setReplaceId(const QString &replaceId)
{
    if (m_replaceId != replaceId) {
        m_replaceId = replaceId;
        Q_EMIT replaceIdChanged();
    }
}

void MessageComposition::setReplyToJid(const QString &replyToJid)
{
    if (m_replyToJid != replyToJid) {
        m_replyToJid = replyToJid;
        Q_EMIT replyToJidChanged();
    }
}

void MessageComposition::setReplyToGroupChatParticipantId(const QString &replyToGroupChatParticipantId)
{
    if (m_replyToGroupChatParticipantId != replyToGroupChatParticipantId) {
        m_replyToGroupChatParticipantId = replyToGroupChatParticipantId;
        Q_EMIT replyToGroupChatParticipantIdChanged();
    }
}

void MessageComposition::setReplyToName(const QString &replyToName)
{
    if (m_replyToName != replyToName) {
        m_replyToName = replyToName;
        Q_EMIT replyToNameChanged();
    }
}

void MessageComposition::setReplyId(const QString &replyId)
{
    if (m_replyId != replyId) {
        m_replyId = replyId;
        Q_EMIT replyIdChanged();
    }
}

void MessageComposition::setOriginalReplyId(const QString &originalReplyId)
{
    if (m_originalReplyId != originalReplyId) {
        m_originalReplyId = originalReplyId;
        Q_EMIT originalReplyIdChanged();
    }
}

void MessageComposition::setReplyQuote(const QString &replyQuote)
{
    if (m_replyQuote != replyQuote) {
        m_replyQuote = replyQuote;
        Q_EMIT replyQuoteChanged();
    }
}

void MessageComposition::setBody(const QString &body)
{
    if (m_body != body) {
        m_body = body;
        Q_EMIT bodyChanged();
    }
}

void MessageComposition::setOriginalBody(const QString &originalBody)
{
    if (m_originalBody != originalBody) {
        m_originalBody = originalBody;
        Q_EMIT originalBodyChanged();
    }
}

void MessageComposition::setSpoiler(bool spoiler)
{
    if (m_isSpoiler != spoiler) {
        m_isSpoiler = spoiler;
        Q_EMIT isSpoilerChanged();
    }
}

void MessageComposition::setSpoilerHint(const QString &spoilerHint)
{
    if (m_spoilerHint != spoilerHint) {
        m_spoilerHint = spoilerHint;
        Q_EMIT spoilerHintChanged();
    }
}

void MessageComposition::setIsDraft(bool isDraft)
{
    if (m_isDraft != isDraft) {
        m_isDraft = isDraft;
        Q_EMIT isDraftChanged();

        if (!isDraft) {
            MessageDb::instance()->removeDraftMessage(m_accountJid, m_chatJid);
        }
    }
}

void MessageComposition::send()
{
    Q_ASSERT(!m_accountJid.isNull());
    Q_ASSERT(!m_chatJid.isNull());

    Message message;

    message.accountJid = m_accountJid;
    message.chatJid = m_chatJid;
    message.isOwn = true;

    const auto rosterItem = RosterModel::instance()->findItem(m_chatJid);

    if (rosterItem->isGroupChat()) {
        message.groupChatSenderId = rosterItem->groupChatParticipantId;
    }

    const auto originId = QXmppUtils::generateStanzaUuid();
    message.id = originId;
    message.originId = originId;
    setReply(message, m_replyToJid, m_replyToGroupChatParticipantId, m_replyId, m_replyQuote);
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.setUnpreparedBody(m_body);
    message.encryption = ChatController::instance()->activeEncryption();
    message.deliveryState = DeliveryState::Pending;
    message.isSpoiler = m_isSpoiler;
    message.spoilerHint = m_spoilerHint;
    message.files = m_fileSelectionModel->files();
    message.receiptRequested = true;

    // generate file IDs if needed
    if (m_fileSelectionModel->hasFiles()) {
        message.fileGroupId = MessageDb::instance()->newFileGroupId();

        for (auto &file : message.files) {
            file.fileGroupId = *message.fileGroupId;
            file.id = MessageDb::instance()->newFileId();
            file.name = QUrl::fromLocalFile(file.localFilePath).fileName();
        }
    }

    // add pending message to database
    MessageDb::instance()->addMessage(message, MessageOrigin::UserInput);

    if (m_fileSelectionModel->hasFiles()) {
        // upload files and send message after uploading

        // whether to symmetrically encrypt the files
        bool encrypt = message.encryption != Encryption::NoEncryption;

        auto *fSController = Kaidan::instance()->fileSharingController();
        // upload files
        fSController->sendFiles(message.files, encrypt).then(fSController, [message = std::move(message)](auto result) mutable {
            if (auto files = std::get_if<QList<File>>(&result)) {
                // uploading succeeded

                // set updated files with new metadata and uploaded sources
                message.files = std::move(*files);

                // update message in database
                MessageDb::instance()->updateMessage(message.id, [files = message.files](auto &message) {
                    message.files = files;
                });

                // send message with file sources
                MessageController::instance()->sendPendingMessage(std::move(message));
            } else {
                // uploading did not succeed
                auto errorText = std::get<QXmppError>(std::move(result)).description;

                // set error text in database
                MessageDb::instance()->updateMessage(message.id, [errorText](auto &message) {
                    message.errorText = tr("Upload failed: %1").arg(errorText);
                });
            }
        });
        m_fileSelectionModel->clear();
    } else {
        // directly send message
        MessageController::instance()->sendPendingMessage(std::move(message));
    }

    clear();
}

void MessageComposition::correct()
{
    MessageDb::instance()->updateMessage(
        m_replaceId,
        [this,
         replaceId = m_replaceId,
         replyToJid = m_replyToJid,
         replyToGroupChatParticipantId = m_replyToGroupChatParticipantId,
         replyId = m_replyId,
         replyQuote = m_replyQuote,
         body = m_body,
         spoilerHint = m_spoilerHint](Message &message) {
            setReply(message, replyToJid, replyToGroupChatParticipantId, replyId, replyQuote);
            message.setUnpreparedBody(body);
            message.isSpoiler = !spoilerHint.isEmpty();
            message.spoilerHint = spoilerHint;

            if (message.deliveryState != Enums::DeliveryState::Pending) {
                message.id = QXmppUtils::generateStanzaUuid();

                // Set "replaceId" only on first correction.
                // That way, "replaceId" stays the ID of the originally corrected message.
                if (message.replaceId.isEmpty()) {
                    message.replaceId = replaceId;
                }

                message.deliveryState = Enums::DeliveryState::Pending;

                runOnThread(this, [this, message]() mutable {
                    if (ConnectionState(Kaidan::instance()->connectionState()) == Enums::ConnectionState::StateConnected) {
                        // the trick with the time is important for the servers
                        // this way they can tell which version of the message is the latest
                        message.timestamp = QDateTime::currentDateTimeUtc();

                        await(MessageController::instance()->send(message.toQXmpp()), this, [messageId = message.id](QXmpp::SendResult &&result) {
                            if (std::holds_alternative<QXmppError>(result)) {
                                // TODO store in the database only error codes, assign text messages right in the QML
                                Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Message correction was not successful"));

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
                });
            } else if (message.replaceId.isEmpty()) {
                message.timestamp = QDateTime::currentDateTimeUtc();
            }
        });

    clear();
}

void MessageComposition::clear()
{
    setReplaceId({});
    setReplyToJid({});
    setReplyToGroupChatParticipantId({});
    setReplyToName({});
    setReplyId({});
    setReplyQuote({});
    setBody({});
    setOriginalBody({});
    setSpoiler(false);
    setSpoilerHint({});
    setIsDraft(false);
}

void MessageComposition::setReply(Message &message,
                                  const QString &replyToJid,
                                  const QString &replyToGroupChatParticipantId,
                                  const QString &replyId,
                                  const QString &replyQuote)
{
    if (replyId.isEmpty()) {
        message.reply.reset();
    } else {
        Message::Reply reply;

        reply.toJid = replyToJid;
        reply.toGroupChatParticipantId = replyToGroupChatParticipantId;
        reply.id = replyId;
        reply.quote = replyQuote;

        message.reply = reply;
    }
}

void MessageComposition::loadDraft()
{
    if (m_accountJid.isEmpty() || m_chatJid.isEmpty()) {
        return;
    }

    auto future = MessageDb::instance()->fetchDraftMessage(m_accountJid, m_chatJid);
    await(future, this, [this](std::optional<Message> &&message) {
        if (message) {
            if (const auto replaceId = message->replaceId; !replaceId.isEmpty()) {
                setReplaceId(replaceId);

                await(MessageDb::instance()->fetchMessage(m_accountJid, m_chatJid, replaceId), this, [this](std::optional<Message> &&message) {
                    if (message) {
                        if (const auto &reply = message->reply) {
                            setOriginalReplyId(reply->id);
                        }

                        setOriginalBody(message->body());

                        Q_EMIT isDraftChanged();
                    }
                });
            }

            if (const auto reply = message->reply) {
                setReplyToJid(reply->toJid);
                setReplyToGroupChatParticipantId(reply->toGroupChatParticipantId);

                // Only process if it is not a reply to an own message.
                if (!(m_replyToJid.isEmpty() && m_replyToGroupChatParticipantId.isEmpty())) {
                    if (const auto rosterItem = RosterModel::instance()->findItem(m_chatJid); rosterItem->isGroupChat()) {
                        await(GroupChatUserDb::instance()->user(m_accountJid, m_chatJid, m_replyToGroupChatParticipantId),
                              this,
                              [this](const std::optional<GroupChatUser> &user) {
                                  setReplyToName(user ? user->displayName() : m_replyToGroupChatParticipantId);
                                  Q_EMIT isDraftChanged();
                              });
                    } else {
                        setReplyToName(rosterItem->displayName());
                    }
                }

                setReplyId(reply->id);
                setReplyQuote(reply->quote);
            }

            setBody(message->body());
            setSpoiler(message->isSpoiler);
            setSpoilerHint(message->spoilerHint);
            setIsDraft(true);
        }
    });
}

void MessageComposition::saveDraft()
{
    if (m_accountJid.isEmpty() || m_chatJid.isEmpty()) {
        return;
    }

    const bool savingNeeded = !m_body.isEmpty() || !m_spoilerHint.isEmpty();

    if (m_isDraft) {
        if (savingNeeded) {
            MessageDb::instance()->updateDraftMessage(m_accountJid,
                                                      m_chatJid,
                                                      [this,
                                                       replaceId = m_replaceId,
                                                       replyToJid = m_replyToJid,
                                                       replyToGroupChatParticipantId = m_replyToGroupChatParticipantId,
                                                       replyId = m_replyId,
                                                       replyQuote = m_replyQuote,
                                                       body = m_body,
                                                       isSpoiler = m_isSpoiler,
                                                       spoilerHint = m_spoilerHint](Message &message) {
                                                          message.replaceId = replaceId;
                                                          setReply(message, replyToJid, replyToGroupChatParticipantId, replyId, replyQuote);
                                                          message.timestamp = QDateTime::currentDateTimeUtc();
                                                          message.setUnpreparedBody(body);
                                                          message.isSpoiler = isSpoiler;
                                                          message.spoilerHint = spoilerHint;
                                                      });
        } else {
            MessageDb::instance()->removeDraftMessage(m_accountJid, m_chatJid);
        }
    } else if (savingNeeded) {
        Message message;
        message.accountJid = m_accountJid;
        message.chatJid = m_chatJid;
        message.replaceId = m_replaceId;
        setReply(message, m_replyToJid, m_replyToGroupChatParticipantId, m_replyId, m_replyQuote);
        message.timestamp = QDateTime::currentDateTimeUtc();
        message.setUnpreparedBody(m_body);
        message.isSpoiler = m_isSpoiler;
        message.spoilerHint = m_spoilerHint;
        message.deliveryState = DeliveryState::Draft;

        MessageDb::instance()->addDraftMessage(message);
    }
}

FileSelectionModel::FileSelectionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

FileSelectionModel::~FileSelectionModel() = default;

QHash<int, QByteArray> FileSelectionModel::roleNames() const
{
    static const QHash<int, QByteArray> roles{
        {Name, QByteArrayLiteral("name")},
        {Description, QByteArrayLiteral("description")},
        {Size, QByteArrayLiteral("size")},
        {LocalFileUrl, QByteArrayLiteral("localFileUrl")},
        {PreviewImage, QByteArrayLiteral("previewImage")},
        {Type, QByteArrayLiteral("type")},
    };
    return roles;
}

int FileSelectionModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_files.size();
}

QVariant FileSelectionModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        return {};
    }

    const auto &file = m_files.at(index.row());

    switch (role) {
    case Name:
        return QUrl::fromLocalFile(file.localFilePath).fileName();
    case Description:
        if (file.description) {
            return *file.description;
        }
        return QString();
    case Size:
        if (file.size) {
            return file.formattedSize();
        }
        return tr("Unknown size");
    case LocalFileUrl:
        return file.localFileUrl();
    case PreviewImage:
        if (const auto previewImage = file.previewImage(); !previewImage.isNull()) {
            return previewImage;
        }

        return file.mimeTypeIcon();
    case Type:
        return QVariant::fromValue(file.type());
    }

    return {};
}

void FileSelectionModel::selectFile()
{
    const auto files = QFileDialog::getOpenFileUrls();

    for (const auto &file : files) {
        addFile(file);
    }
}

void FileSelectionModel::addFile(const QUrl &localFileUrl, bool isNew)
{
    auto localPath = localFileUrl.toLocalFile();

    bool alreadyAdded = containsIf(m_files, [=](const auto &file) {
        return file.localFilePath == localPath;
    });

    if (alreadyAdded) {
        return;
    }

    Q_ASSERT(localFileUrl.isLocalFile());
    const auto limit = Kaidan::instance()->serverFeaturesCache()->httpUploadLimit();
    const QFileInfo fileInfo(localPath);

    if (fileInfo.size() > limit) {
        Kaidan::instance()->passiveNotificationRequested(tr("'%1' cannot be sent because it is larger than %2")
                                                             .arg(fileInfo.fileName(), Kaidan::instance()->serverFeaturesCache()->httpUploadLimitString()));
        return;
    }

    File file;
    file.localFilePath = localPath;
    file.mimeType = MediaUtils::mimeDatabase().mimeTypeForFile(localPath);
    file.size = fileInfo.size();
    file.isNew = isNew;

    auto insertFile = [this](File &&file) mutable {
        beginInsertRows({}, m_files.size(), m_files.size());
        m_files.append(std::move(file));
        endInsertRows();
    };

    if (file.type() == MessageType::MessageVideo) {
        await(MediaUtils::generateThumbnail(file.localFileUrl(), file.mimeTypeName(), VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT),
              this,
              [file, insertFile](const QByteArray &thumbnail) mutable {
                  file.thumbnail = thumbnail;
                  insertFile(std::move(file));
              });
    } else {
        insertFile(std::move(file));
    }
}

void FileSelectionModel::removeFile(int index)
{
    deleteNewFile(m_files.at(index));

    beginRemoveRows({}, index, index);
    m_files.remove(index);
    endRemoveRows();
}

void FileSelectionModel::deleteNewFiles()
{
    std::for_each(m_files.cbegin(), m_files.cend(), deleteNewFile);
}

void FileSelectionModel::clear()
{
    beginResetModel();
    m_files.clear();
    endResetModel();
}

bool FileSelectionModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    auto &file = m_files[index.row()];

    switch (role) {
    case Description:
        file.description = value.toString();
        break;
    default:
        return false;
    }

    return true;
}

const QList<File> &FileSelectionModel::files() const
{
    return m_files;
}

void FileSelectionModel::deleteNewFile(const File &file)
{
    if (file.isNew) {
        MediaUtils::deleteFile(file.localFileUrl());
    }
}

#include "moc_MessageComposition.cpp"
