// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "FileProgressCache.h"

#include <QCoreApplication>

FileProgressWatcher::FileProgressWatcher(QObject *parent)
	: QObject(parent)
{
}

FileProgressWatcher::~FileProgressWatcher() = default;

QString FileProgressWatcher::messageId() const
{
	return m_key.value_or(QString());
}

void FileProgressWatcher::setMessageId(QString mId)
{
	if (!m_key || (m_key && *m_key != mId)) {
		setKey(std::move(mId));
		emit messageIdChanged();

		auto progress = FileProgressCache::instance().progress(*m_key);
		m_loading = progress.has_value();
		m_progress = progress.value_or(FileProgress());
		emit progressChanged();
	}
}

void FileProgressWatcher::notify(const std::optional<FileProgress> &value)
{
	m_loading = value.has_value();
	m_progress = value.value_or(FileProgress());
	emit progressChanged();
}

FileProgressCache::FileProgressCache() = default;

FileProgressCache::~FileProgressCache()
{
	// wait for other threads to finish
	std::scoped_lock locker(m_mutex);
}

FileProgressCache &FileProgressCache::instance()
{
	static FileProgressCache cache;
	return cache;
}

std::optional<FileProgress> FileProgressCache::progress(const QString &messageId)
{
	std::scoped_lock locker(m_mutex);
	if (auto itr = m_files.find(messageId); itr != m_files.end()) {
		return itr->second;
	}
	return {};
}

void FileProgressCache::reportProgress(const QString &messageId, std::optional<FileProgress> progress)
{
	std::scoped_lock locker(m_mutex);
	if (progress) {
		m_files.insert_or_assign(messageId, *progress);
	} else {
		m_files.erase(messageId);
	}
	// watchers live on main thread
	QMetaObject::invokeMethod(QCoreApplication::instance(), [messageId, progress] {
		FileProgressNotifier::instance().notifyWatchers(messageId, progress);
	});
}
