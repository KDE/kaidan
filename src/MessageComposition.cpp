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
#include <QImageReader>
// KDE
#include <KFileItem>
#include <KIO/PreviewJob>
// QXmpp
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "ChatController.h"
#include "FileSharingController.h"
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "ImageProvider.h"
#include "MainController.h"
#include "MediaUtils.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "QmlUtils.h"
#include "RosterModel.h"

MessageComposition::MessageComposition()
    : m_fileSelectionModel(new FileSelectionModel(this))
{
    connect(qGuiApp, &QGuiApplication::aboutToQuit, this, [this]() {
        saveDraft();
    });
}

void MessageComposition::setChatController(ChatController *chatController)
{
    if (m_chatController != chatController) {
        m_chatController = chatController;

        connect(m_chatController, &ChatController::aboutToChangeChat, this, [this]() {
            if (m_chatController->account()) {
                saveDraft();
                clear();
            }
        });

        connect(m_chatController, &ChatController::chatChanged, this, [this]() {
            loadDraft().then([this]() {
                if (const auto messageBodyToForward = m_chatController->messageBodyToForward(); !messageBodyToForward.isEmpty()) {
                    const auto forwardedMessage = [this, messageBodyToForward]() {
                        const QString quotedBody = m_chatController->account()->settings()->displayName() + u":\n" + QmlUtils::quote(messageBodyToForward);

                        if (m_body.isEmpty()) {
                            return quotedBody;
                        } else {
                            return QString{m_body + u"\n" + quotedBody};
                        }
                    }();

                    setBody(forwardedMessage);
                    m_chatController->setMessageBodyToForward({});
                    setIsForwarding(true);
                }

                Q_EMIT preparedForNewChat();
            });
        });

        connect(m_chatController, &ChatController::accountChanged, this, [this]() {
            auto *account = m_chatController->account();

            m_connection = account->connection();
            m_fileSharingController = account->fileSharingController();
            m_messageController = account->messageController();

            m_fileSelectionModel->setAccountSettings(account->settings());
        });
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
    }
}

void MessageComposition::send()
{
    Q_ASSERT(!m_chatController->account()->settings()->jid().isNull());
    Q_ASSERT(!m_chatController->jid().isNull());

    Message message;

    message.accountJid = m_chatController->account()->settings()->jid();
    message.chatJid = m_chatController->jid();
    message.isOwn = true;

    const auto rosterItem = RosterModel::instance()->item(m_chatController->account()->settings()->jid(), m_chatController->jid());

    if (rosterItem->isGroupChat()) {
        message.groupChatSenderId = rosterItem->groupChatParticipantId;
    }

    const auto originId = QXmppUtils::generateStanzaUuid();
    message.id = originId;
    message.originId = originId;
    setReply(message, m_replyToJid, m_replyToGroupChatParticipantId, m_replyId, m_replyQuote);
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.setUnpreparedBody(m_body);
    message.encryption = m_chatController->activeEncryption();
    message.deliveryState = DeliveryState::Pending;
    message.isSpoiler = m_isSpoiler;
    message.spoilerHint = m_spoilerHint;
    message.files = m_fileSelectionModel->files();
    message.receiptRequested = true;

    // generate file IDs if needed
    if (!message.files.isEmpty()) {
        message.fileGroupId = MessageDb::instance()->newFileGroupId();

        for (auto &file : message.files) {
            file.fileGroupId = *message.fileGroupId;
            file.id = MessageDb::instance()->newFileId();
            file.name = QUrl::fromLocalFile(file.localFilePath).fileName();
        }
    }

    // add pending message to database
    MessageDb::instance()->addMessage(message, MessageOrigin::UserInput);

    m_messageController->sendPendingMessage(std::move(message));

    reset();
}

void MessageComposition::correct()
{
    MessageDb::instance()->updateMessage(
        m_chatController->account()->settings()->jid(),
        m_chatController->jid(),
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

                QMetaObject::invokeMethod(this, [this, message]() mutable {
                    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
                        auto sendMessage = [this](Message &&message, const QList<QString> &encryptionJids = {}) {
                            m_messageController->send(message.toQXmpp(), message.encryption, encryptionJids)
                                .then([accountJid = message.accountJid, chatJid = message.chatJid, messageId = message.id](QXmpp::SendResult &&result) {
                                    if (std::holds_alternative<QXmppError>(result)) {
                                        // TODO store in the database only error codes, assign text messages right in the
                                        // QML
                                        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Message correction was not successful"));

                                        MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [](Message &message) {
                                            message.deliveryState = DeliveryState::Error;
                                            message.errorText = QStringLiteral("Message correction was not successful");
                                        });
                                    } else {
                                        MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [](Message &message) {
                                            message.deliveryState = DeliveryState::Sent;
                                            message.errorText.clear();
                                        });
                                    }
                                });
                        };

                        if (message.encryption != Encryption::NoEncryption && message.isGroupChatMessage()) {
                            sendMessage(std::move(message), m_chatController->groupChatUserJids());
                        } else {
                            sendMessage(std::move(message));
                        }
                    }
                });
            }
        });

    reset();
}

void MessageComposition::clear()
{
    setReplaceId({});
    setReplyToJid({});
    setReplyToGroupChatParticipantId({});
    setReplyToName({});
    setReplyId({});
    setReplyQuote({});
    setOriginalBody({});
    setSpoiler(false);
    setIsForwarding(false);

    m_fileSelectionModel->clear();
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

QFuture<void> MessageComposition::loadDraft()
{
    return MessageDb::instance()
        ->fetchDraftMessage(m_chatController->account()->settings()->jid(), m_chatController->jid())
        .then(this, [this](std::optional<Message> &&message) {
            if (message) {
                if (const auto replaceId = message->replaceId; !replaceId.isEmpty()) {
                    setReplaceId(replaceId);

                    MessageDb::instance()
                        ->fetchMessage(m_chatController->account()->settings()->jid(), m_chatController->jid(), replaceId)
                        .then(this, [this](std::optional<Message> &&message) {
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
                        if (const auto rosterItem = RosterModel::instance()->item(m_chatController->account()->settings()->jid(), m_chatController->jid());
                            rosterItem->isGroupChat()) {
                            GroupChatUserDb::instance()
                                ->user(m_chatController->account()->settings()->jid(), m_chatController->jid(), m_replyToGroupChatParticipantId)
                                .then(this, [this](const std::optional<GroupChatUser> &user) {
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
    if (m_chatController->account()->settings()->jid().isEmpty() || m_chatController->jid().isEmpty()) {
        return;
    }

    const bool savingNeeded = !m_body.isEmpty() || !m_spoilerHint.isEmpty();

    if (m_isDraft) {
        if (savingNeeded) {
            MessageDb::instance()->updateDraftMessage(m_chatController->account()->settings()->jid(),
                                                      m_chatController->jid(),
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
            MessageDb::instance()->removeDraftMessage(m_chatController->account()->settings()->jid(), m_chatController->jid());
            setIsDraft(false);
        }
    } else if (savingNeeded) {
        Message message;
        message.accountJid = m_chatController->account()->settings()->jid();
        message.chatJid = m_chatController->jid();
        message.replaceId = m_replaceId;
        setReply(message, m_replyToJid, m_replyToGroupChatParticipantId, m_replyId, m_replyQuote);
        message.timestamp = QDateTime::currentDateTimeUtc();
        message.setUnpreparedBody(m_body);
        message.isSpoiler = m_isSpoiler;
        message.spoilerHint = m_spoilerHint;
        message.deliveryState = DeliveryState::Draft;

        MessageDb::instance()->addDraftMessage(message);

        setIsDraft(true);
    }
}

void MessageComposition::setIsForwarding(bool isForwarding)
{
    if (m_isForwarding != isForwarding) {
        m_isForwarding = isForwarding;
        Q_EMIT isForwardingChanged();
    }
}

void MessageComposition::reset()
{
    if (m_isDraft) {
        MessageDb::instance()->removeDraftMessage(m_chatController->account()->settings()->jid(), m_chatController->jid());
    }

    clear();
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
        {PreviewImageUrl, QByteArrayLiteral("previewImageUrl")},
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
    case PreviewImageUrl:
        return ImageProvider::instance()->generatedFileImageUrl(file);
    case Type:
        return QVariant::fromValue(file.type());
    }

    return {};
}

void FileSelectionModel::setAccountSettings(AccountSettings *accountSettings)
{
    m_accountSettings = accountSettings;
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
    const auto limit = m_accountSettings->httpUploadLimit();
    const QFileInfo fileInfo(localPath);

    if (fileInfo.size() > limit) {
        Q_EMIT MainController::instance()->passiveNotificationRequested(
            tr("'%1' cannot be sent because it is larger than %2").arg(fileInfo.fileName(), m_accountSettings->httpUploadLimitText()));
        return;
    }

    File file;
    file.localFilePath = localPath;
    file.mimeType = MediaUtils::mimeDatabase().mimeTypeForFile(localPath);
    file.size = fileInfo.size();
    file.isNew = isNew;
    file.transferOutgoing = true;

    if (auto image = QImageReader(localPath); image.canRead()) {
        const auto size = [&image]() {
            QSize size = image.size();

            if (!size.isValid()) {
                size = image.read().size();
            }

            return size;
        }();

        file.width = size.width();
        file.height = size.height();
    }

    const int row = m_files.size();
    beginInsertRows({}, row, row);
    m_files.append(std::move(file));
    endInsertRows();
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
        QFile::remove(file.localFilePath);
    }
}

#include "moc_MessageComposition.cpp"
