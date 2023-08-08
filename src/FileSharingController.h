// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QXmppTask.h>

#include <QXmppFileSharingManager.h>

struct File;
struct Message;
class QXmppFileSharingManager;
class QXmppHttpFileSharingProvider;
class QXmppClient;

class FileSharingController : public QObject
{
	Q_OBJECT
public:
	using SendFilesResult = std::variant<QVector<File>, QXmppError>;
	using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

	explicit FileSharingController(QXmppClient *client);

	static qint64 generateFileId();

	auto sendFiles(QVector<File> files, bool encrypt) -> QXmppTask<SendFilesResult>;
	Q_INVOKABLE void downloadFile(const QString &messageId, const File &file);
	Q_INVOKABLE void deleteFile(const QString &messageId, const File &file);

	Q_SIGNAL void errorOccured(qint64, QXmppError);

private:
	QFuture<UploadResult> sendFile(const File &file, bool encrypt);
};
