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
#include <QMimeDatabase>
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
#include <QXmppMessage.h>
#include <QXmppOutOfBandUrl.h>
#include <QXmppPromise.h>
#include <QXmppUploadRequestManager.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "FileProgressCache.h"
#include "FutureUtils.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "SystemUtils.h"

using std::ranges::all_of;
using std::ranges::find;
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

    const auto isBadChar = [=](QChar c) {
        return find(bad_chars, c) != bad_chars.end() || c.category() == QChar::Other_Control;
    };
    const auto isBadName = [=](QStringView name) {
        return find(bad_names, name) != bad_names.end();
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

FileSharingController::FileSharingController(AccountSettings *accountSettings,
                                             Connection *connection,
                                             MessageController *messageController,
                                             ClientWorker *clientWorker,
                                             QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_messageController(messageController)
    , m_clientWorker(clientWorker)
    , m_manager(clientWorker->fileSharingManager())
{
    auto *uploadRequestManager = m_clientWorker->uploadRequestManager();

    connect(uploadRequestManager, &QXmppUploadRequestManager::serviceFoundChanged, this, [this, uploadRequestManager, connection]() {
        if (connection->state() == Enums::ConnectionState::StateConnected) {
            runOnThread(
                uploadRequestManager,
                [uploadRequestManager]() {
                    return uploadRequestManager->uploadServices();
                },
                this,
                [this](QVector<QXmppUploadService> &&uploadServices) {
                    const auto limit = uploadServices.isEmpty() ? 0 : uploadServices.constFirst().sizeLimit();
                    m_accountSettings->setHttpUploadLimit(limit);
                });
        }
    });
}

void FileSharingController::sendFile(const QString &chatJid, const QString &messageId, const File &file, bool encrypt)
{
    sendFileTask(chatJid, messageId, file, encrypt).then([=, this](bool ok) {
        if (ok) {
            maybeSendPendingMessage(chatJid, messageId);
        }
    });
}

void FileSharingController::sendFiles(const QString &chatJid, const QString &messageId, const QList<File> &files, bool encrypt)
{
    join(this,
         transform(files,
                   [=, this](const auto &file) {
                       return sendFileTask(chatJid, messageId, file, encrypt);
                   }))
        .then([=, this](const QList<bool> oks) {
            if (all_true(oks)) {
                maybeSendPendingMessage(chatJid, messageId);
            }
        });
}

void FileSharingController::removeFile(const QString &filePath)
{
    // Only remove files that were downloaded by the user.
    if (filePath.startsWith(SystemUtils::downloadDirectory())) {
        QFile::remove(filePath);
    }
}

void FileSharingController::maybeSendPendingMessage(const QString &chatJid, const QString &messageId)
{
    MessageDb::instance()->fetchMessage(m_accountSettings->jid(), chatJid, messageId).then([=, this](std::optional<Message> &&message) {
        if (message) {
            if (all_of(message->files, [](const auto &f) {
                    return f.transferState == File::TransferState::Done;
                })) {
                m_messageController->sendPendingMessage(std::move(*message));
            }
        }
    });
}

void FileSharingController::downloadFile(const QString &chatJid, const QString &messageId, const File &file)
{
    runOnThread(m_manager, [this, chatJid, messageId, fileId = file.id, fileShare = file.toQXmpp()] {
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

        MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [=](Message &message) {
            auto file = find_if(message.files, [=](const auto &file) {
                return file.id == fileId;
            });

            if (file != message.files.end()) {
                file->transferState = File::TransferState::Transferring;
            }
        });

        auto download = m_manager->downloadFile(fileShare, std::move(output));

        std::weak_ptr<QXmppFileDownload> downloadPtr = download;
        connect(download.get(), &QXmppFileDownload::progressChanged, this, [=]() {
            if (auto download = downloadPtr.lock()) {
                if (auto progress = FileProgressCache::instance().progress(fileId)) {
                    if (progress->canceled) {
                        download->cancel();
                        return;
                    }
                }

                FileProgressCache::instance().reportProgress(fileId,
                                                             FileProgress{download->bytesTransferred(), quint64(download->bytesTotal()), download->progress()});
            }
        });

        connect(download.get(), &QXmppFileDownload::finished, this, [this, chatJid, messageId, fileId, filePath, download]() mutable {
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
                MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [=](Message &message) {
                    auto file = find_if(message.files, [=](const auto &file) {
                        return file.id == fileId;
                    });

                    if (file != message.files.end()) {
                        file->localFilePath = filePath;
                        file->transferState = File::TransferState::Done;
                    }
                    // TODO: generate possibly missing metadata
                    // metadata may be missing if the sender only used out of band urls
                });
            } else if (error) {
                const bool canceled = error->isNetworkError() && error->value<QNetworkReply::NetworkError>() == QNetworkReply::OperationCanceledError;
                const auto errorText = error->description;

                MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [=](Message &message) {
                    auto file = find_if(message.files, [=](const auto &file) {
                        return file.id == fileId;
                    });

                    if (file != message.files.end()) {
                        file->transferState = canceled ? File::TransferState::Canceled : File::TransferState::Failed;
                    }
                });

                qCDebug(KAIDAN_CORE_LOG) << "Could not download file:" << errorText;
                removeFile(filePath);

                if (!canceled) {
                    Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not download file: %1").arg(errorText));
                }
            }

            FileProgressCache::instance().reportProgress(fileId, {});
            // reduce ref count
            download.reset();
        });
    });
}

void FileSharingController::deleteFile(const QString &chatJid, const QString &messageId, const File &file)
{
    MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [fileId = file.id](Message &message) {
        auto it = find_if(message.files, [fileId](const auto &file) {
            return file.id == fileId;
        });
        if (it != message.files.end()) {
            it->localFilePath.clear();
            it->transferState = File::TransferState::Pending;
        }
    });

    removeFile(file.localFilePath);
}

void FileSharingController::cancelFile(const File &file)
{
    auto progress = FileProgressCache::instance().progress(file.id);

    if (progress) {
        auto updatedProgress = *progress;
        updatedProgress.canceled = true;

        FileProgressCache::instance().reportProgress(file.id, updatedProgress);
    }
}

QFuture<bool> FileSharingController::sendFileTask(const QString &chatJid, const QString &messageId, const File &file, bool encrypt)
{
    auto promise = std::make_shared<QPromise<bool>>();

    runOnThread(m_manager, [=, this]() mutable {
        auto provider = encrypt ? std::static_pointer_cast<QXmppFileSharingProvider>(m_clientWorker->encryptedHttpFileSharingProvider())
                                : std::static_pointer_cast<QXmppFileSharingProvider>(m_clientWorker->httpFileSharingProvider());

        MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [fileId = file.id](Message &message) {
            auto file = find_if(message.files, [=](const auto &file) {
                return file.id == fileId;
            });

            if (file != message.files.end()) {
                file->transferState = File::TransferState::Transferring;
            }
        });

        auto upload = m_manager->uploadFile(provider, file.localFilePath, file.description);

        FileProgressCache::instance().reportProgress(file.id, FileProgress{0, quint64(upload->bytesTotal()), 0.0F});

        std::weak_ptr<QXmppFileUpload> uploadPtr = upload;
        connect(upload.get(), &QXmppFileUpload::progressChanged, this, [=] {
            if (auto upload = uploadPtr.lock()) {
                if (auto progress = FileProgressCache::instance().progress(file.id)) {
                    if (progress->canceled) {
                        upload->cancel();
                        return;
                    }
                }

                FileProgressCache::instance().reportProgress(file.id,
                                                             FileProgress{upload->bytesTransferred(), quint64(upload->bytesTotal()), upload->progress()});
            }
        });

        connect(upload.get(), &QXmppFileUpload::finished, this, [=, this]() mutable {
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
                    ->updateMessage(m_accountSettings->jid(),
                                    chatJid,
                                    messageId,
                                    [=, fileResult = *fileResult](Message &message) {
                                        auto it = find_if(message.files, [=](const auto &f) {
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

                                        if (all_of(message.files, [](const auto &f) {
                                                return f.transferState == File::TransferState::Done;
                                            })) {
                                            message.errorText.clear();
                                        }
                                    })
                    .then([promise]() {
                        promise->addResult(true);
                        promise->finish();
                    });
            } else if (error) {
                const bool canceled = error->isNetworkError() && error->value<QNetworkReply::NetworkError>() == QNetworkReply::OperationCanceledError;
                const auto errorText = error->description;

                MessageDb::instance()
                    ->updateMessage(m_accountSettings->jid(),
                                    chatJid,
                                    messageId,
                                    [=](Message &message) {
                                        auto it = find_if(message.files, [=](const auto &f) {
                                            return f.id == file.id;
                                        });

                                        if (it != message.files.end()) {
                                            it->transferState = canceled ? File::TransferState::Canceled : File::TransferState::Failed;
                                        }

                                        message.errorText = tr("Upload failed: %1").arg(errorText);
                                    })
                    .then([promise]() {
                        promise->addResult(false);
                        promise->finish();
                    });

                qCDebug(KAIDAN_CORE_LOG) << "Could not upload file:" << errorText;

                if (!canceled) {
                    Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not upload file: %1").arg(errorText));
                }
            }

            FileProgressCache::instance().reportProgress(file.id, std::nullopt);
            // reduce ref count
            upload.reset();
        });
    });

    return promise->future();
}

#include "moc_FileSharingController.cpp"
