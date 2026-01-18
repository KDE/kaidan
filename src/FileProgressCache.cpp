// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileProgressCache.h"

FileProgressWatcher::FileProgressWatcher(QObject *parent)
    : QObject(parent)
{
}

FileProgressWatcher::~FileProgressWatcher() = default;

QString FileProgressWatcher::fileId() const
{
    return QString::number(m_key.value_or(0));
}

void FileProgressWatcher::setFileId(const QString &fileId)
{
    qint64 fId = fileId.toLongLong();
    if (!m_key || (m_key && *m_key != fId)) {
        setKey(std::move(fId));
        Q_EMIT fileIdChanged();

        auto progress = FileProgressCache::instance().progress(*m_key);
        m_loading = progress.has_value();
        m_progress = progress.value_or(FileProgress());
        Q_EMIT progressChanged();
    }
}

void FileProgressWatcher::notify(const std::optional<FileProgress> &value)
{
    m_loading = value.has_value();
    m_progress = value.value_or(FileProgress());
    Q_EMIT progressChanged();
}

FileProgressCache::FileProgressCache() = default;

FileProgressCache &FileProgressCache::instance()
{
    static FileProgressCache cache;
    return cache;
}

std::optional<FileProgress> FileProgressCache::progress(qint64 fileId)
{
    if (auto itr = m_files.find(fileId); itr != m_files.end()) {
        return itr->second;
    }

    return {};
}

void FileProgressCache::reportProgress(qint64 fileId, const FileProgress &progress)
{
    m_files.insert_or_assign(fileId, progress);
    FileProgressNotifier::instance().notifyWatchers(fileId, progress);
}

void FileProgressCache::reportFinished(qint64 fileId)
{
    m_files.erase(fileId);
    FileProgressNotifier::instance().notifyWatchers(fileId, {});
}

void FileProgressCache::cancelTransfers(const QString &accountJid)
{
    const auto files = m_files;

    for (const auto &file : files) {
        if (file.second.accountJid == accountJid) {
            if (file.second.cancel) {
                file.second.cancel();
            }
        }
    }
}

#include "moc_FileProgressCache.cpp"
