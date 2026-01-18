// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// QXmpp
#include <QXmppFileSharingManager.h>
#include <QXmppHttpUploadManager.h>
#include <QXmppTask.h>
// Kaidan
#include <Message.h>

class AccountSettings;
class ClientController;
class Connection;
struct File;
struct FileProgress;
class QXmppClient;
class Message;

class FileSharingController : public QObject
{
    Q_OBJECT
public:
    using SendFileResult = std::variant<File, QXmppError>;
    using SendFilesResult = std::unordered_map<qint64, SendFileResult>;
    using UploadResult = std::tuple<qint64, QXmppFileUpload::Result>;

    FileSharingController(AccountSettings *accountSettings, Connection *connection, ClientController *clientController, QObject *parent = nullptr);

    void sendPendingFiles(const QString &chatJid, const QString &messageId, const QList<File> &files, bool encrypt);
    Q_SIGNAL void filesUploadedForPendingMessage(const Message &message);

    Q_INVOKABLE void sendFile(const QString &chatJid, const QString &messageId, const File &file, bool encrypt);
    Q_INVOKABLE void downloadFile(const QString &chatJid, const QString &messageId, const File &file);
    Q_INVOKABLE void cancelTransfer(const QString &chatJid, const QString &messageId, const File &file);

private:
    QFuture<bool> sendFileTask(const QString &chatJid, const QString &messageId, const File &file, bool encrypt);
    void maybeSendPendingMessage(const QString &chatJid, const QString &messageId);
    static void resetError(Message &message);
    static bool checkAllTransfersCompleted(const Message &message);

    void downloadPendingFiles();

    void handleUploadSupportChanged();
    void handleMessageAdded(const Message &message, MessageOrigin origin);
    QFuture<void> handleTransferError(const QString &chatJid,
                                      const QString &messageId,
                                      qint64 fileId,
                                      const FileProgress &progress,
                                      const QXmppError &error,
                                      std::function<void()> &&handleFailure);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    ClientController *const m_clientController;
    QXmppFileSharingManager *const m_manager;
    QXmppHttpUploadManager::Support m_uploadSupport;
};
