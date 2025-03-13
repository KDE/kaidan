// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileModel.h"

#include "FutureUtils.h"
#include "MediaUtils.h"
#include "MessageDb.h"
#include "kaidan_core_debug.h"

FileModel::FileModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(&m_watcher, &QFutureWatcher<Files>::finished, this, [this]() {
        setFiles(m_watcher.result());
    });
}

FileModel::~FileModel()
{
    m_watcher.cancel();
    m_watcher.waitForFinished();
}

int FileModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? m_files.size() : 0;
}

QVariant FileModel::data(const QModelIndex &index, int role) const
{
    if (hasIndex(index.row(), index.column())) {
        const auto &file = m_files[index.row()];

        switch (role) {
        case Qt::DisplayRole:
            return file.httpSources.isEmpty() ? QString() : file.httpSources.constBegin()->url.toDisplayString();
        case static_cast<int>(Role::Id):
            return file.id;
        case static_cast<int>(Role::File):
            return QVariant::fromValue(file);
        case static_cast<int>(Role::Thumbnail): {
            const auto url = file.localFileUrl();

            // If the local file does not exist, use a stored thumbnail.
            if (!url.isValid()) {
                return file.thumbnail.isEmpty() ? QVariant{} : QImage::fromData(file.thumbnail);
            }

            if (const auto image = QImage::fromData(file.thumbnail); image.width() >= VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT) {
                return image;
            }

            auto fileModel = const_cast<FileModel *>(this);

            await(MediaUtils::generateThumbnail(url, file.mimeTypeName(), VIDEO_THUMBNAIL_EDGE_PIXEL_COUNT),
                  fileModel,
                  [fileModel, pIndex = QPersistentModelIndex(index), fileId = file.fileId()](const QByteArray &thumbnail) mutable {
                      if (!pIndex.isValid()) {
                          return;
                      }

                      auto &file = fileModel->m_files[pIndex.row()];

                      if (file.fileId() != fileId) {
                          return;
                      }

                      file.thumbnail = thumbnail;

                      Q_EMIT fileModel->dataChanged(pIndex,
                                                    pIndex,
                                                    {
                                                        static_cast<int>(Role::File),
                                                        static_cast<int>(Role::Thumbnail),
                                                    });
                  });
        }
        }
    }

    return {};
}

QHash<int, QByteArray> FileModel::roleNames() const
{
    static const auto roles = [this]() {
        // The ID is used by C++ but QML does not understand qint64.
        // Thus, it is added to "data()" but not here.
        auto roles = QAbstractListModel::roleNames();
        roles.insert(static_cast<int>(Role::File), QByteArrayLiteral("file"));
        roles.insert(static_cast<int>(Role::Thumbnail), QByteArrayLiteral("thumbnail"));
        return roles;
    }();
    return roles;
}

QString FileModel::accountJid() const
{
    return m_accountJid;
}

void FileModel::setAccountJid(const QString &jid)
{
    if (m_accountJid != jid) {
        m_accountJid = jid;
        Q_EMIT accountJidChanged(m_accountJid);
    }
}

QString FileModel::chatJid() const
{
    return m_chatJid;
}

void FileModel::setChatJid(const QString &jid)
{
    if (m_chatJid != jid) {
        m_chatJid = jid;
        Q_EMIT chatJidChanged(m_chatJid);
    }
}

Files FileModel::files() const
{
    return m_files;
}

void FileModel::setFiles(const Files &files)
{
    if (m_files != files) {
        beginResetModel();
        m_files = files;
        endResetModel();

        Q_EMIT rowCountChanged();
    }
}

void FileModel::loadFiles()
{
    m_watcher.cancel();

    if (m_accountJid.isEmpty()) {
        qCWarning(KAIDAN_CORE_LOG, "FileModel: Trying to call loadFiles() but m_accountJid is empty.");
    } else if (m_chatJid.isEmpty()) {
        m_watcher.setFuture(MessageDb::instance()->fetchFiles(m_accountJid));
    } else {
        m_watcher.setFuture(MessageDb::instance()->fetchFiles(m_accountJid, m_chatJid));
    }
}

void FileModel::loadDownloadedFiles()
{
    m_watcher.cancel();

    if (m_accountJid.isEmpty()) {
        qCWarning(KAIDAN_CORE_LOG,
                  "FileModel: Trying to call loadDownloadedFiles() but m_accountJid is "
                  "empty.");
    } else if (m_chatJid.isEmpty()) {
        m_watcher.setFuture(MessageDb::instance()->fetchDownloadedFiles(m_accountJid));
    } else {
        m_watcher.setFuture(MessageDb::instance()->fetchDownloadedFiles(m_accountJid, m_chatJid));
    }
}

#include "moc_FileModel.cpp"
