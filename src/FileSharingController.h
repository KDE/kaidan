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

class AccountSettings;
class ClientWorker;
class Connection;
struct File;
class QXmppClient;

class FileSharingController : public QObject
{
    Q_OBJECT
public:
    using SendFilesResult = std::variant<QList<File>, QXmppError>;
    using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

    FileSharingController(AccountSettings *accountSettings, Connection *connection, ClientWorker *clientWorker, QObject *parent = nullptr);

    auto sendFiles(QList<File> files, bool encrypt) -> QXmppTask<SendFilesResult>;
    Q_INVOKABLE void downloadFile(const QString &chatJid, const QString &messageId, const File &file);
    Q_INVOKABLE void deleteFile(const QString &chatJid, const QString &messageId, const File &file);
    Q_INVOKABLE void cancelFile(const File &file);

private:
    QFuture<UploadResult> sendFile(const File &file, bool encrypt);
    void removeFile(const QString &filePath);

    AccountSettings *const m_accountSettings;
    ClientWorker *const m_clientWorker;
    QXmppFileSharingManager *const m_manager;
};
