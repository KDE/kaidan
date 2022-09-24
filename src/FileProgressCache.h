// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <mutex>

#include "AbstractNotifier.h"

struct FileProgress
{
	quint64 bytesSent = 0;
	quint64 bytesTotal = 0;
	float progress = 0;
};

using FileProgressNotifier = AbstractNotifier<QString, std::optional<FileProgress>>;

class FileProgressWatcher : public QObject, public AbstractWatcher<QString, std::optional<FileProgress>>
{
	Q_OBJECT
	Q_PROPERTY(QString messageId READ messageId WRITE setMessageId NOTIFY messageIdChanged)
	Q_PROPERTY(bool isLoading READ isLoading NOTIFY progressChanged)
	Q_PROPERTY(quint64 bytesSent READ bytesSent NOTIFY progressChanged)
	Q_PROPERTY(quint64 bytesTotal READ bytesTotal NOTIFY progressChanged)
	Q_PROPERTY(float progress READ progress NOTIFY progressChanged)

public:
	explicit FileProgressWatcher(QObject *parent = nullptr);
	~FileProgressWatcher() override;

	[[nodiscard]] QString messageId() const;
	void setMessageId(QString);
	Q_SIGNAL void messageIdChanged();

	[[nodiscard]] bool isLoading() const { return m_loading; }
	[[nodiscard]] quint64 bytesSent() const { return m_progress.bytesSent; }
	[[nodiscard]] quint64 bytesTotal() const { return m_progress.bytesTotal; }
	[[nodiscard]] float progress() const { return m_progress.progress; }

	Q_SIGNAL void progressChanged();

private:
	void notify(const std::optional<FileProgress> &value) override;
	bool m_loading = false;
	FileProgress m_progress;
};

class FileProgressCache
{
public:
	~FileProgressCache();

	static FileProgressCache &instance();

	std::optional<FileProgress> progress(const QString &messageId);
	void reportProgress(const QString &messageId, std::optional<FileProgress> progress);

private:
	FileProgressCache();

	std::mutex m_mutex;
	std::unordered_map<QString, FileProgress> m_files;
};
