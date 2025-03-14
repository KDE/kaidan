// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// QXmpp
#include <QXmppFileSharingManager.h>
#include <QXmppTask.h>

struct File;
class QXmppClient;

class FileSharingController : public QObject
{
    Q_OBJECT
public:
    using SendFilesResult = std::variant<QList<File>, QXmppError>;
    using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

    explicit FileSharingController(QXmppClient *client);

    auto sendFiles(QList<File> files, bool encrypt) -> QXmppTask<SendFilesResult>;
    Q_INVOKABLE void downloadFile(const QString &messageId, const File &file);
    Q_INVOKABLE void deleteFile(const QString &messageId, const File &file);

    Q_SIGNAL void errorOccured(qint64, QXmppError);

private:
    QFuture<UploadResult> sendFile(const File &file, bool encrypt);
};
