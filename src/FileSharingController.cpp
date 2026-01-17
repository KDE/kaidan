// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileSharingController.h"

// std
#include <array>
// Qt
#include <QDir>
#include <QImage>
#include <QNetworkReply>
#include <QStandardPaths>
#include <QStringBuilder>
// KDE
#include <KFileUtils>
// QXmpp
#include <QXmppBitsOfBinaryDataList.h>
#include <QXmppEncryptedFileSharingProvider.h>
#include <QXmppError.h>
#include <QXmppFileMetadata.h>
#include <QXmppHash.h>
#include <QXmppHttpFileSharingProvider.h>
#include <QXmppHttpFileSource.h>
#include <QXmppHttpUploadManager.h>
#include <QXmppMessage.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppPromise.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "FileProgressCache.h"
#include "FutureUtils.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "MediaUtils.h"
#include "MessageDb.h"
#include "RosterModel.h"
#include "SystemUtils.h"

using std::ranges::all_of;
using std::ranges::find_if;

// ported from https://github.com/SergioBenitez/Rocket/blob/2cee4b459492136d616e5863c54754b135e41572/core/lib/src/fs/file_name.rs#L112
///
/// Removes problematic (as in reserved characters, filenames with special meaning etc.)
/// parts from a given file name, and returns only the meaningful name (without the file extension)
///
/// A file extension can be added again by inferring it from the mime type if one is needed.
///
static std::optional<std::pair<QString, QString>> sanitizeFilename(QStringView fileName)
{
    constexpr std::array bad_chars = {
#ifdef Q_OS_UNIX
        // These have special meaning in a file name.
        QLatin1Char('.'),
        QLatin1Char('/'),
        QLatin1Char('\\'),

        // These are treated specially by shells.
        QLatin1Char('<'),
        QLatin1Char('>'),
        QLatin1Char('|'),
        QLatin1Char(':'),
        QLatin1Char('('),
        QLatin1Char(')'),
        QLatin1Char('&'),
        QLatin1Char(';'),
        QLatin1Char('#'),
        QLatin1Char('?'),
        QLatin1Char('*'),
#else
        // Microsoft says these are invalid.
        QLatin1Char('.'),
        QLatin1Char('<'),
        QLatin1Char('>'),
        QLatin1Char(':'),
        QLatin1Char('"'),
        QLatin1Char('/'),
        QLatin1Char('\\'),
        QLatin1Char('|'),
        QLatin1Char('?'),
        QLatin1Char('*'),

        // `cmd.exe` treats these specially.
        QLatin1Char(','),
        QLatin1Char(';'),
        QLatin1Char('='),

        // These are treated specially by unix-like shells.
        QLatin1Char('('),
        QLatin1Char(')'),
        QLatin1Char('&'),
        QLatin1Char('#'),
#endif
    };

    std::initializer_list<QStringView> bad_names = {
#ifndef Q_OS_UNIX
        u"CON",  u"PRN",  u"AUX",  u"NUL",  u"COM1", u"COM2", u"COM3", u"COM4", u"COM5", u"COM6", u"COM7",
        u"COM8", u"COM9", u"LPT1", u"LPT2", u"LPT3", u"LPT4", u"LPT5", u"LPT6", u"LPT7", u"LPT8", u"LPT9",
#endif
    };

    const auto isBadChar = [bad_chars](QChar c) {
        return std::ranges::contains(bad_chars, c) || c.category() == QChar::Other_Control;
    };
    const auto isBadName = [bad_names](QStringView name) {
        return std::ranges::contains(bad_names, name);
    };

    // Tokenize file name by splitting on bad chars
    QStringList filenameParts;
    QString substr;
    for (auto c : fileName) {
        if (isBadChar(c)) {
            filenameParts.push_back(substr);
            substr.clear();
        } else {
            substr.push_back(c);
        }
    }
    if (!substr.isEmpty()) {
        filenameParts.push_back(substr);
    }

    auto relevantPart = find_if(filenameParts, [](const auto &part) {
        return !part.isEmpty();
    });

    // No substring we can use, filename only contains bad chars
    if (relevantPart == filenameParts.end()) {
        return {};
    }

    QString fileExtension;
    for (auto itr = --filenameParts.end(); itr != relevantPart; itr--) {
        if (!itr->isEmpty()) {
            fileExtension = *itr;
            break;
        }
    }

    auto filename = *relevantPart;

    if (isBadName(filename)) {
        return {};
    }
    return std::pair{filename, fileExtension};
}

FileSharingController::FileSharingController(AccountSettings *accountSettings, Connection *connection, ClientController *clientController, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_clientController(clientController)
    , m_manager(clientController->fileSharingManager())
{
    connect(m_connection, &Connection::stateChanged, this, [this]() {
        if (m_connection->state() == Enums::ConnectionState::StateConnected) {
            downloadPendingFiles();
        } else if (m_connection->state() == Enums::ConnectionState::StateDisconnected) {
            FileProgressCache::instance().cancelTransfers(m_accountSettings->jid());
        }
    });
    connect(m_clientController->uploadManager(), &QXmppHttpUploadManager::supportChanged, this, &FileSharingController::handleUploadSupportChanged);
    connect(MessageDb::instance(), &MessageDb::messageAdded, this, &FileSharingController::handleMessageAdded);
}

void FileSharingController::sendPendingFiles(const QString &chatJid, const QString &messageId, const QList<File> &files, bool encrypt)
{
    join(this,
         transform(files,
                   [this, chatJid, messageId, encrypt](const auto &file) {
                       if (file.transferState == File::TransferState::Pending) {
                           return sendFileTask(chatJid, messageId, file, encrypt);
                       }

                       auto promise = std::make_shared<QPromise<bool>>();
                       promise->start();
                       promise->addResult(file.transferState == File::TransferState::Done);
                       promise->finish();
                       return promise->future();
                   }))
        .then([this, chatJid, messageId](const QList<bool> oks) {
            if (all_true(oks)) {
                maybeSendPendingMessage(chatJid, messageId);
            }
        });
}

void FileSharingController::sendFile(const QString &chatJid, const QString &messageId, const File &file, bool encrypt)
{
    if (file.transferState == File::TransferState::Done) {
        maybeSendPendingMessage(chatJid, messageId);
    } else {
        sendFileTask(chatJid, messageId, file, encrypt).then([this, chatJid, messageId](bool ok) {
            if (ok) {
                maybeSendPendingMessage(chatJid, messageId);
            }
        });
    }
}

void FileSharingController::downloadFile(const QString &chatJid, const QString &messageId, const File &file)
{
    const auto accountJid = m_accountSettings->jid();
    const auto fileId = file.id;

    if (m_connection->state() != Enums::ConnectionState::StateConnected) {
        MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [fileId = file.id](Message &message) {
            auto file = find_if(message.files, [fileId](const auto &file) {
                return file.id == fileId;
            });

            if (file != message.files.end()) {
                file->transferState = File::TransferState::Pending;
            }
        });

        return;
    }

    const auto fileShare = file.toQXmpp();
    QString dirPath = SystemUtils::downloadDirectory();

    if (auto dir = QDir(dirPath); !dir.exists()) {
        dir.mkpath(QStringLiteral("."));
    }

    // Sanitize file name, if given
    auto maybeFileName = andThen(fileShare.metadata().filename(), sanitizeFilename);

    // Add fallback file name, so we always have a file name
    auto fileName = [&]() {
        if (maybeFileName) {
            return maybeFileName->first;
        }
        return QDateTime::currentDateTime().toString();
    }();

    const auto fileExtension = [&]() {
        auto extension = fileShare.metadata().mediaType()->preferredSuffix();
        if (!extension.isEmpty()) {
            return extension;
        }
        if (maybeFileName) {
            return maybeFileName->second;
        }
        return QString();
    }();

    const QString fileNameWithExtension = fileName + QLatin1Char('.') + fileExtension;

    // Ensure that the file name is locally unique.
    const auto uniqueFileNameWithExtension = KFileUtils::suggestName(QUrl::fromLocalFile(dirPath), fileNameWithExtension);
    const QString filePath = dirPath + QDir::separator() + uniqueFileNameWithExtension;

    // Open the file at the resulting path
    auto output = std::make_unique<QFile>(filePath);

    if (!output->open(QIODevice::WriteOnly)) {
        qCDebug(KAIDAN_CORE_LOG) << "Failed to open output file at" << filePath;
        return;
    }

    MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [fileId](Message &message) {
        auto file = find_if(message.files, [fileId](const auto &file) {
            return file.id == fileId;
        });

        if (file != message.files.end()) {
            file->transferState = File::TransferState::Transferring;
        }
    });

    auto download = m_manager->downloadFile(fileShare, std::move(output));
    std::weak_ptr<QXmppFileDownload> downloadPtr = download;

    FileProgressCache::instance().reportProgress(fileId,
                                                 FileProgress{
                                                     accountJid,
                                                     0,
                                                     download->bytesTotal(),
                                                     0.0F,
                                                     [downloadPtr]() {
                                                         if (auto download = downloadPtr.lock()) {
                                                             download->cancel();
                                                         }
                                                     },
                                                 });

    connect(download.get(), &QXmppFileDownload::progressChanged, this, [fileId, downloadPtr]() {
        if (auto download = downloadPtr.lock()) {
            auto progress = FileProgressCache::instance().progress(fileId);
            Q_ASSERT(progress);

            progress->bytesSent = download->bytesTransferred();
            progress->bytesTotal = download->bytesTotal();
            progress->progress = download->progress();

            FileProgressCache::instance().reportProgress(fileId, *progress);
        }
    });

    connect(download.get(), &QXmppFileDownload::finished, this, [this, accountJid, chatJid, messageId, fileId, filePath, download]() mutable {
        auto result = download->result();
        auto error = [&result]() -> std::optional<QXmppError> {
            if (std::holds_alternative<QXmpp::Cancelled>(result)) {
                return QXmppError{tr("Canceled"), QNetworkReply::OperationCanceledError};
            } else if (std::holds_alternative<QXmppError>(result)) {
                return std::get<QXmppError>(result);
            }

            return {};
        }();

        if (std::holds_alternative<QXmppFileDownload::Downloaded>(result)) {
            MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [fileId, filePath](Message &message) {
                auto file = find_if(message.files, [fileId](const auto &file) {
                    return file.id == fileId;
                });

                if (file != message.files.end()) {
                    file->localFilePath = filePath;
                    file->transferState = File::TransferState::Done;
                }

                resetError(message);

                // TODO: generate possibly missing metadata
                // metadata may be missing if the sender only used out of band urls
            });
        } else if (error) {
            auto progress = FileProgressCache::instance().progress(fileId);
            Q_ASSERT(progress);

            const auto errorText = error->description;

            handleTransferError(chatJid, messageId, fileId, *progress, *error, [errorText]() {
                Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not download file: %1").arg(errorText));
            });

            qCDebug(KAIDAN_CORE_LOG) << "Could not download file:" << errorText;

            MediaUtils::deleteDownloadedFile(filePath);
        }

        FileProgressCache::instance().reportFinished(fileId);
        // reduce ref count
        download.reset();
    });
}

void FileSharingController::cancelTransfer(const QString &chatJid, const QString &messageId, const File &file)
{
    auto progress = FileProgressCache::instance().progress(file.id);

    if (progress && progress->cancel) {
        progress->canceledByUser = true;
        FileProgressCache::instance().reportProgress(file.id, *progress);

        progress->cancel();
    } else {
        MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [fileId = file.id](Message &message) {
            auto file = find_if(message.files, [fileId](const auto &file) {
                return file.id == fileId;
            });

            if (file != message.files.end()) {
                file->transferState = File::TransferState::CanceledByUser;
            }
        });
    }
}

QFuture<bool> FileSharingController::sendFileTask(const QString &chatJid, const QString &messageId, const File &file, bool encrypt)
{
    const auto updateMessageError = [accountJid = m_accountSettings->jid(), chatJid, messageId](const QString &text) {
        MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [text](Message &message) {
            message.errorText = text;
        });
    };

    const auto accountJid = m_accountSettings->jid();
    switch (m_uploadSupport) {
    case QXmppHttpUploadManager::Support::Unknown:
        updateMessageError(tr("Media sharing support is unknown"));
        return QtFuture::makeReadyValueFuture(false);
    case QXmppHttpUploadManager::Support::Unsupported:
        updateMessageError(tr("Media sharing is unsupported"));
        return QtFuture::makeReadyValueFuture(false);
    case QXmppHttpUploadManager::Support::Supported:
        break;
    }

    auto promise = std::make_shared<QPromise<bool>>();

    auto provider = encrypt ? std::static_pointer_cast<QXmppFileSharingProvider>(m_clientController->encryptedHttpFileSharingProvider())
                            : std::static_pointer_cast<QXmppFileSharingProvider>(m_clientController->httpFileSharingProvider());

    MessageDb::instance()->updateMessage(accountJid, chatJid, messageId, [fileId = file.id](Message &message) {
        auto file = find_if(message.files, [fileId](const auto &file) {
            return file.id == fileId;
        });

        if (file != message.files.end()) {
            file->transferState = File::TransferState::Transferring;
        }
    });

    auto upload = m_manager->uploadFile(provider, file.localFilePath, file.description);
    std::weak_ptr<QXmppFileUpload> uploadPtr = upload;

    FileProgressCache::instance().reportProgress(file.id,
                                                 FileProgress{
                                                     accountJid,
                                                     0,
                                                     upload->bytesTotal(),
                                                     0.0F,
                                                     [uploadPtr]() {
                                                         if (auto upload = uploadPtr.lock()) {
                                                             upload->cancel();
                                                         }
                                                     },
                                                 });

    connect(upload.get(), &QXmppFileUpload::progressChanged, this, [file, uploadPtr] {
        if (auto upload = uploadPtr.lock()) {
            auto progress = FileProgressCache::instance().progress(file.id);
            Q_ASSERT(progress);

            progress->bytesSent = upload->bytesTransferred();
            progress->bytesTotal = upload->bytesTotal();
            progress->progress = upload->progress();

            FileProgressCache::instance().reportProgress(file.id, *progress);
        }
    });

    auto handleUploadFinished = [this, accountJid, chatJid, messageId, file, upload, promise]() mutable {
        auto result = upload->result();
        auto error = [&result]() -> std::optional<QXmppError> {
            if (std::holds_alternative<QXmpp::Cancelled>(result)) {
                return QXmppError{tr("Canceled"), QNetworkReply::OperationCanceledError};
            } else if (std::holds_alternative<QXmppError>(result)) {
                return std::get<QXmppError>(result);
            }

            return std::nullopt;
        }();

        if (auto fileResult = std::get_if<QXmppFileUpload::FileResult>(&result)) {
            MessageDb::instance()
                ->updateMessage(accountJid,
                                chatJid,
                                messageId,
                                [file, fileResult = *fileResult](Message &message) {
                                    auto it = find_if(message.files, [file](const auto &f) {
                                        return f.id == file.id;
                                    });

                                    if (it != message.files.end()) {
                                        it->transferState = File::TransferState::Done;

                                        if (!fileResult.dataBlobs.empty()) {
                                            it->thumbnail = fileResult.dataBlobs.constFirst().data();
                                        }

                                        it->httpSources = transform(fileResult.fileShare.httpSources(), [&](const auto &s) {
                                            return HttpSource{it->id, s.url()};
                                        });

                                        it->encryptedSources = transform(fileResult.fileShare.encryptedSources(), [&](const auto &s) {
                                            QUrl sourceUrl;
                                            if (!s.httpSources().empty()) {
                                                sourceUrl = s.httpSources().first().url();
                                            }

                                            std::optional<qint64> encryptedDataId;
                                            if (!s.hashes().empty()) {
                                                encryptedDataId = MessageDb::instance()->newFileId();
                                            }

                                            return EncryptedSource{it->id,
                                                                   sourceUrl,
                                                                   s.cipher(),
                                                                   s.key(),
                                                                   s.iv(),
                                                                   encryptedDataId,
                                                                   transform(s.hashes(), [&](const auto &hash) {
                                                                       return FileHash{encryptedDataId.value(), hash.algorithm(), hash.hash()};
                                                                   })};
                                        });

                                        it->hashes = transform(fileResult.fileShare.metadata().hashes(), [&](const auto &hash) {
                                            return FileHash{it->id, hash.algorithm(), hash.hash()};
                                        });
                                    }

                                    resetError(message);
                                })
                .then([promise]() {
                    promise->addResult(true);
                    promise->finish();
                });
        } else if (error) {
            auto progress = FileProgressCache::instance().progress(file.id);
            Q_ASSERT(progress);

            const auto errorText = error->description;

            handleTransferError(chatJid, messageId, file.id, *progress, *error, [errorText]() {
                Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not upload file: %1").arg(errorText));
            }).then([promise]() {
                promise->addResult(false);
                promise->finish();
            });

            qCDebug(KAIDAN_CORE_LOG) << "Could not upload file:" << errorText;
        }

        FileProgressCache::instance().reportFinished(file.id);
        // reduce ref count
        upload.reset();
    };

    connect(upload.get(), &QXmppFileUpload::finished, this, handleUploadFinished);

    if (upload->isFinished()) {
        handleUploadFinished();
    }

    return promise->future();
}

void FileSharingController::maybeSendPendingMessage(const QString &chatJid, const QString &messageId)
{
    MessageDb::instance()->fetchMessage(m_accountSettings->jid(), chatJid, messageId).then([this](std::optional<Message> &&message) {
        if (message) {
            if (checkAllTransfersCompleted(*message)) {
                Q_EMIT filesUploadedForPendingMessage(*message);
            }
        }
    });
}

void FileSharingController::resetError(Message &message)
{
    if (checkAllTransfersCompleted(message)) {
        message.errorText.clear();
    }
}

bool FileSharingController::checkAllTransfersCompleted(const Message &message)
{
    return all_of(message.files, [](const auto &file) {
        return file.transferState == File::TransferState::Done;
    });
}

void FileSharingController::downloadPendingFiles()
{
    MessageDb::instance()->fetchAutomaticallyDownloadableFiles(m_accountSettings->jid()).then(this, [this](QList<MessageDb::DownloadableFile> &&files) {
        for (MessageDb::DownloadableFile &file : files) {
            downloadFile(file.chatJid, file.messageId, file.file);
        }
    });
}

void FileSharingController::handleUploadSupportChanged()
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        auto *uploadManager = m_clientController->uploadManager();

        if (const auto services = uploadManager->services(); !services.isEmpty()) {
            const auto limit = services.constFirst().sizeLimit();
            m_accountSettings->setHttpUploadLimit(limit ? static_cast<qint64>(*limit) : -1);
        }

        m_uploadSupport = uploadManager->support();
    } else {
        m_uploadSupport = QXmppHttpUploadManager::Support::Unknown;
    }
}

void FileSharingController::handleMessageAdded(const Message &message, MessageOrigin origin)
{
    if (origin != MessageOrigin::UserInput && message.accountJid == m_accountSettings->jid() && !message.files.isEmpty()) {
        if (RosterModel::instance()->item(message.accountJid, message.chatJid)->automaticDownloadsEnabled()) {
            for (const auto &file : message.files) {
                if (file.localFilePath.isEmpty() || !QFile::exists(file.localFilePath)) {
                    downloadFile(message.chatJid, message.id, file);
                }
            }
        } else {
            MessageDb::instance()->updateMessage(message.accountJid, message.chatJid, message.relevantId(), [](Message &message) {
                for (auto &file : message.files) {
                    file.transferState = File::TransferState::Done;
                }
            });
        }
    }
}

QFuture<void> FileSharingController::handleTransferError(const QString &chatJid,
                                                         const QString &messageId,
                                                         qint64 fileId,
                                                         const FileProgress &progress,
                                                         const QXmppError &error,
                                                         std::function<void()> &&handleFailure)
{
    return MessageDb::instance()->updateMessage(
        m_accountSettings->jid(),
        chatJid,
        messageId,
        [fileId,
         errorText = error.description,
         canceled = error.isNetworkError() && error.value<QNetworkReply::NetworkError>() == QNetworkReply::OperationCanceledError,
         canceledByUser = progress.canceledByUser,
         handleFailure](Message &message) {
            auto it = find_if(message.files, [fileId](const auto &file) {
                return file.id == fileId;
            });

            if (it != message.files.end()) {
                if (canceledByUser) {
                    it->transferState = File::TransferState::CanceledByUser;
                    message.errorText = tr("Transfer canceled");
                } else if (canceled) {
                    // The file transfer is started again once connected to the server.
                    it->transferState = File::TransferState::Pending;
                } else {
                    it->transferState = File::TransferState::Failed;
                    message.errorText = tr("Transfer failed: %1").arg(errorText);
                    handleFailure();
                }
            }
        });
}

#include "moc_FileSharingController.cpp"
