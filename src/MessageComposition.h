// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
#include <QFuture>
// Kaidan
#include "Encryption.h"

struct File;

class AccountSettings;
class ChatController;
class Connection;
class FileSelectionModel;
class FileSharingController;
class Message;
class MessageController;

class MessageComposition : public QObject
{
    Q_OBJECT

    Q_PROPERTY(ChatController *chatController MEMBER m_chatController WRITE setChatController)

    Q_PROPERTY(FileSelectionModel *fileSelectionModel MEMBER m_fileSelectionModel CONSTANT)

    Q_PROPERTY(QString replaceId READ replaceId WRITE setReplaceId NOTIFY replaceIdChanged)
    Q_PROPERTY(QString replyToJid READ replyToJid WRITE setReplyToJid NOTIFY replyToJidChanged)
    Q_PROPERTY(QString replyToGroupChatParticipantId READ replyToGroupChatParticipantId WRITE setReplyToGroupChatParticipantId NOTIFY
                   replyToGroupChatParticipantIdChanged)
    Q_PROPERTY(QString replyToName READ replyToName WRITE setReplyToName NOTIFY replyToNameChanged)
    Q_PROPERTY(QString replyId READ replyId WRITE setReplyId NOTIFY replyIdChanged)
    Q_PROPERTY(QString originalReplyId READ originalReplyId WRITE setOriginalReplyId NOTIFY originalReplyIdChanged)
    Q_PROPERTY(QString replyQuote READ replyQuote WRITE setReplyQuote NOTIFY replyQuoteChanged)
    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
    Q_PROPERTY(QString originalBody READ originalBody WRITE setOriginalBody NOTIFY originalBodyChanged)
    Q_PROPERTY(bool isSpoiler READ isSpoiler WRITE setSpoiler NOTIFY isSpoilerChanged)
    Q_PROPERTY(QString spoilerHint READ spoilerHint WRITE setSpoilerHint NOTIFY spoilerHintChanged)
    Q_PROPERTY(bool isDraft READ isDraft WRITE setIsDraft NOTIFY isDraftChanged)
    Q_PROPERTY(bool isForwarding READ isForwarding NOTIFY isForwardingChanged)

public:
    MessageComposition();
    ~MessageComposition() override = default;

    void setChatController(ChatController *chatController);

    [[nodiscard]] QString replaceId() const
    {
        return m_replaceId;
    }
    void setReplaceId(const QString &replaceId);

    [[nodiscard]] QString replyToJid() const
    {
        return m_replyToJid;
    }
    void setReplyToJid(const QString &replyToJid);

    [[nodiscard]] QString replyToGroupChatParticipantId() const
    {
        return m_replyToGroupChatParticipantId;
    }
    void setReplyToGroupChatParticipantId(const QString &replyToGroupChatParticipantId);

    [[nodiscard]] QString replyToName() const
    {
        return m_replyToName;
    }
    void setReplyToName(const QString &replyToName);

    [[nodiscard]] QString replyId() const
    {
        return m_replyId;
    }
    void setReplyId(const QString &replyId);

    [[nodiscard]] QString originalReplyId() const
    {
        return m_originalReplyId;
    }
    void setOriginalReplyId(const QString &originalReplyId);

    [[nodiscard]] QString replyQuote() const
    {
        return m_replyQuote;
    }
    void setReplyQuote(const QString &replyQuote);

    [[nodiscard]] QString body() const
    {
        return m_body;
    }
    void setBody(const QString &body);

    [[nodiscard]] QString originalBody() const
    {
        return m_originalBody;
    }
    void setOriginalBody(const QString &originalBody);

    [[nodiscard]] bool isSpoiler() const
    {
        return m_isSpoiler;
    }
    void setSpoiler(bool spoiler);

    [[nodiscard]] QString spoilerHint() const
    {
        return m_spoilerHint;
    }
    void setSpoilerHint(const QString &spoilerHint);

    [[nodiscard]] bool isDraft() const
    {
        return m_isDraft;
    }
    void setIsDraft(bool isDraft);

    [[nodiscard]] bool isForwarding() const
    {
        return m_isForwarding;
    }

    Q_INVOKABLE void send();
    Q_INVOKABLE void correct();

    Q_INVOKABLE void clear();

    Q_SIGNAL void accountJidChanged();
    Q_SIGNAL void chatJidChanged();
    Q_SIGNAL void replaceIdChanged();
    Q_SIGNAL void replyToJidChanged();
    Q_SIGNAL void replyToGroupChatParticipantIdChanged();
    Q_SIGNAL void replyToNameChanged();
    Q_SIGNAL void replyIdChanged();
    Q_SIGNAL void originalReplyIdChanged();
    Q_SIGNAL void replyQuoteChanged();
    Q_SIGNAL void bodyChanged();
    Q_SIGNAL void originalBodyChanged();
    Q_SIGNAL void isSpoilerChanged();
    Q_SIGNAL void spoilerHintChanged();
    Q_SIGNAL void isDraftChanged();
    Q_SIGNAL void isForwardingChanged();

    Q_SIGNAL void draftSaved();
    Q_SIGNAL void preparedForNewChat();

private:
    void setReply(Message &message, const QString &replyToJid, const QString &replyToGroupChatParticipantId, const QString &replyId, const QString &replyQuote);
    QFuture<void> loadDraft();
    void saveDraft();
    void setIsForwarding(bool isForwarding);

    void reset();

    QString m_replaceId;
    QString m_replyToJid;
    QString m_replyToGroupChatParticipantId;
    QString m_replyToName;
    QString m_replyId;
    QString m_originalReplyId;
    QString m_replyQuote;
    QString m_body;
    QString m_originalBody;
    bool m_isSpoiler = false;
    QString m_spoilerHint;
    bool m_isDraft = false;
    bool m_isForwarding = false;

    Connection *m_connection = nullptr;

    ChatController *m_chatController = nullptr;
    FileSharingController *m_fileSharingController = nullptr;
    MessageController *m_messageController = nullptr;

    FileSelectionModel *const m_fileSelectionModel;
};

class FileSelectionModel : public QAbstractListModel
{
    Q_OBJECT

public:
    enum Roles {
        Name = Qt::UserRole + 1,
        Description,
        Size,
        LocalFileUrl,
        PreviewImageUrl,
        Type,
    };

    explicit FileSelectionModel(QObject *parent = nullptr);
    ~FileSelectionModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    void setAccountSettings(AccountSettings *accountSettings);

    Q_INVOKABLE void selectFile();
    Q_INVOKABLE void addFile(const QUrl &localFileUrl, bool isNew = false);
    Q_INVOKABLE void removeFile(int index);
    Q_INVOKABLE void deleteNewFiles();
    Q_INVOKABLE void clear();
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    const QList<File> &files() const;
    bool hasFiles() const
    {
        return !m_files.empty();
    }

private:
    // Deletes a file that the user created via Kaidan.
    static void deleteNewFile(const File &file);

    AccountSettings *m_accountSettings;
    QList<File> m_files;
};
