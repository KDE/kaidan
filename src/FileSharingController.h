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
// Kaidan
#include <Message.h>

class AccountSettings;
class ClientWorker;
class Connection;
struct File;
class QXmppClient;
class Message;
class MessageController;

class FileSharingController : public QObject
{
    Q_OBJECT
public:
    using SendFileResult = std::variant<File, QXmppError>;
    using SendFilesResult = std::unordered_map<qint64, SendFileResult>;
    using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

    FileSharingController(AccountSettings *accountSettings,
                          Connection *connection,
                          MessageController *messageController,
                          ClientWorker *clientWorker,
                          QObject *parent = nullptr);

    Q_INVOKABLE void sendFile(const QString &chatJid, const QString &messageId, const File &file, bool encrypt);
    void sendFiles(const QString &chatJid, const QString &messageId, const QList<File> &files, bool encrypt);
    Q_INVOKABLE void downloadFile(const QString &chatJid, const QString &messageId, const File &file);
    Q_INVOKABLE void deleteFile(const QString &chatJid, const QString &messageId, const File &file);
    Q_INVOKABLE void cancelFile(const File &file);

private:
    QFuture<bool> sendFileTask(const QString &chatJid, const QString &messageId, const File &file, bool encrypt);
    void removeFile(const QString &filePath);
    void maybeSendPendingMessage(const QString &chatJid, const QString &messageId);

    void handleUploadServicesChanged();
    void handleMessageAdded(const Message &message, MessageOrigin origin);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    MessageController *const m_messageController;
    ClientWorker *const m_clientWorker;
    QXmppFileSharingManager *const m_manager;
};
