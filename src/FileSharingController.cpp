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
#include "MessageDb.h"
#include "SystemUtils.h"

template<typename T, typename Lambda>
auto find_if(T &container, Lambda lambda)
{
    return std::find_if(container.begin(), container.end(), std::forward<Lambda>(lambda));
}

template<typename T, typename Value>
auto find(T &container, Value value)
{
    return std::find(container.begin(), container.end(), std::forward<Value>(value));
}

// ported from https://github.com/SergioBenitez/Rocket/blob/2cee4b459492136d616e5863c54754b135e41572/core/lib/src/fs/file_name.rs#L112
///
/// Removes problematic (as in reserved characters, filenames with special meaning etc.)
/// parts from a given file name, and returns only the meaningful name (without the file extension)
///
/// A file extension can be added again by infering it from the mime type if one is needed.
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

FileSharingController::FileSharingController(AccountSettings *accountSettings, Connection *connection, ClientWorker *clientWorker, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
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

auto FileSharingController::sendFiles(QList<File> files, bool encrypt) -> QXmppTask<SendFilesResult>
{
    Q_ASSERT(!files.empty());

    QXmppPromise<SendFilesResult> promise;
    auto task = promise.task();

    auto futures = transform(files, [&](auto &file) {
        return sendFile(file, encrypt);
    });

    join(this, std::move(futures)).then(this, [promise = std::move(promise), files = std::move(files)](QList<UploadResult> &&uploadResults) mutable {
        // Check if any of the uploads failed
        bool failed = std::any_of(uploadResults.begin(), uploadResults.end(), [](const auto &result) {
            auto &[id, uploadResult] = result;
            return !std::holds_alternative<QXmppFileUpload::FileResult>(uploadResult);
        });

        // upload error handling
        if (failed) {
            auto errorIt = find_if(uploadResults, [](const auto &result) {
                auto &[id, uploadResult] = result;
                return std::holds_alternative<QXmppError>(uploadResult);
            });
            Q_ASSERT(errorIt != uploadResults.end());

            // currently this only gives the error of the first failed upload
            promise.finish(std::get<QXmppError>(std::get<1>(std::move(*errorIt))));
            return;
        }

        // extract the file shares from the list of upload results
        auto fileResultsMap = transformMap(uploadResults, [](const auto &result) {
            auto &[fileId, uploadResult] = result;
            return std::pair{fileId, std::get<QXmppFileUpload::FileResult>(uploadResult)};
        });

        for (auto &file : files) {
            auto fileResult = fileResultsMap.find(file.id);
            if (fileResult == fileResultsMap.end()) {
                continue;
            }

            if (!fileResult->second.dataBlobs.empty()) {
                file.thumbnail = fileResult->second.dataBlobs.first().data();
            }

            file.httpSources = transform(fileResult->second.fileShare.httpSources(), [&](const auto &s) {
                return HttpSource{file.id, s.url()};
            });

            file.encryptedSources = transform(fileResult->second.fileShare.encryptedSources(), [&](const auto &s) {
                QUrl sourceUrl;
                if (!s.httpSources().empty()) {
                    sourceUrl = s.httpSources().first().url();
                }

                std::optional<qint64> encryptedDataId;
                if (!s.hashes().empty()) {
                    encryptedDataId = MessageDb::instance()->newFileId();
                }

                return EncryptedSource{file.id, sourceUrl, s.cipher(), s.key(), s.iv(), encryptedDataId, transform(s.hashes(), [&](const auto &hash) {
                                           return FileHash{encryptedDataId.value(), hash.algorithm(), hash.hash()};
                                       })};
            });

            file.hashes = transform(fileResult->second.fileShare.metadata().hashes(), [&](const auto &hash) {
                return FileHash{file.id, hash.algorithm(), hash.hash()};
            });
        }

        promise.finish(std::move(files));
    });

    return task;
}

auto FileSharingController::sendFile(const File &file, bool encrypt) -> QFuture<UploadResult>
{
    QFutureInterface<UploadResult> interface;

    runOnThread(m_manager, [this, file, encrypt, interface]() mutable {
        auto provider = encrypt ? std::static_pointer_cast<QXmppFileSharingProvider>(m_clientWorker->encryptedHttpFileSharingProvider())
                                : std::static_pointer_cast<QXmppFileSharingProvider>(m_clientWorker->httpFileSharingProvider());

        auto upload = m_manager->uploadFile(provider, file.localFilePath, file.description);

        FileProgressCache::instance().reportProgress(file.id, FileProgress{0, quint64(upload->bytesTotal()), 0.0F});

        std::weak_ptr<QXmppFileUpload> uploadPtr = upload;
        connect(upload.get(), &QXmppFileUpload::progressChanged, this, [id = file.id, uploadPtr] {
            if (auto upload = uploadPtr.lock()) {
                FileProgressCache::instance().reportProgress(id, FileProgress{upload->bytesTransferred(), quint64(upload->bytesTotal()), upload->progress()});
            }
        });

        connect(upload.get(), &QXmppFileUpload::finished, this, [this, upload, id = file.id, interface]() mutable {
            auto result = upload->result();

            FileProgressCache::instance().reportProgress(id, std::nullopt);

            if (std::holds_alternative<QXmppError>(result)) {
                Q_EMIT errorOccured(id, std::get<QXmppError>(result));
            }

            interface.reportResult({id, result});
            interface.reportFinished();
            // reduce ref count
            upload.reset();
        });
    });

    return interface.future();
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

        auto download = m_manager->downloadFile(fileShare, std::move(output));

        std::weak_ptr<QXmppFileDownload> downloadPtr = download;
        connect(download.get(), &QXmppFileDownload::progressChanged, this, [=]() {
            if (auto download = downloadPtr.lock()) {
                FileProgressCache::instance().reportProgress(fileId,
                                                             FileProgress{download->bytesTransferred(), quint64(download->bytesTotal()), download->progress()});
            }
        });

        connect(download.get(), &QXmppFileDownload::finished, this, [this, chatJid, messageId, fileId, filePath, download]() mutable {
            auto result = download->result();
            if (std::holds_alternative<QXmppError>(result)) {
                auto errorText = std::get<QXmppError>(result).description;

                qCDebug(KAIDAN_CORE_LOG) << "Could not download file:" << errorText;
                Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not download file: %1").arg(errorText));
            } else if (std::holds_alternative<QXmppFileDownload::Downloaded>(result)) {
                MessageDb::instance()->updateMessage(m_accountSettings->jid(), chatJid, messageId, [=](Message &message) {
                    auto file = find_if(message.files, [=](const auto &file) {
                        return file.id == fileId;
                    });

                    if (file != message.files.cend()) {
                        file->localFilePath = filePath;
                    }
                    // TODO: generate possibly missing metadata
                    // metadata may be missing if the sender only used out of band urls
                });
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
        if (it != message.files.cend()) {
            it->localFilePath.clear();
        }
    });

    // don't delete files not downloaded by us
    if (file.localFilePath.startsWith(SystemUtils::downloadDirectory())) {
        QFile::remove(file.localFilePath);
    }
}

#include "moc_FileSharingController.cpp"
