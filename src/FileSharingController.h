// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>
#include <QFuture>

#include <QXmppFileSharingManager.h>

#include <memory>

struct File;
struct Message;
class QXmppFileSharingManager;
class QXmppHttpFileSharingProvider;
class QXmppClient;

class FileSharingController : public QObject
{
	Q_OBJECT
public:
	using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

	explicit FileSharingController(QXmppClient *client);

	void sendMessage(Message &&message, bool encrypt);

	QFuture<UploadResult> sendFile(const File &file, bool encrypt);
	Q_INVOKABLE void downloadFile(const QString &messageId, const File &file);
	Q_INVOKABLE void deleteFile(const QString &messageId, const File &file);

	Q_SIGNAL void errorOccured(qint64, QXmppError);
};
