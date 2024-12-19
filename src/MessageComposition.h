// SPDX-FileCopyrightText: 2022 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QAbstractListModel>

#include "Message.h"

class FileSelectionModel;
class QFileDialog;
template<typename T>
class QFutureWatcher;

class MessageComposition : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
    Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)
    Q_PROPERTY(QString replaceId READ replaceId WRITE setReplaceId NOTIFY replaceIdChanged)
    Q_PROPERTY(QString replyToJid READ replyToJid WRITE setReplyToJid NOTIFY replyToJidChanged)
    Q_PROPERTY(QString replyToGroupChatParticipantId READ replyToGroupChatParticipantId WRITE setReplyToGroupChatParticipantId NOTIFY
                   replyToGroupChatParticipantIdChanged)
    Q_PROPERTY(QString replyToName READ replyToName WRITE setReplyToName NOTIFY replyToNameChanged)
    Q_PROPERTY(QString replyId READ replyId WRITE setReplyId NOTIFY replyIdChanged)
    Q_PROPERTY(QString replyQuote READ replyQuote WRITE setReplyQuote NOTIFY replyQuoteChanged)
    Q_PROPERTY(QString body READ body WRITE setBody NOTIFY bodyChanged)
    Q_PROPERTY(bool isSpoiler READ isSpoiler WRITE setSpoiler NOTIFY isSpoilerChanged)
    Q_PROPERTY(QString spoilerHint READ spoilerHint WRITE setSpoilerHint NOTIFY spoilerHintChanged)
    Q_PROPERTY(bool isDraft READ isDraft WRITE setIsDraft NOTIFY isDraftChanged)
    Q_PROPERTY(FileSelectionModel *fileSelectionModel MEMBER m_fileSelectionModel CONSTANT)

public:
    MessageComposition();
    ~MessageComposition() override = default;

    [[nodiscard]] QString accountJid() const
    {
        return m_accountJid;
    }
    void setAccountJid(const QString &accountJid);
    [[nodiscard]] QString chatJid() const
    {
        return m_chatJid;
    }
    void setChatJid(const QString &chatJid);
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
    Q_SIGNAL void replyQuoteChanged();
    Q_SIGNAL void bodyChanged();
    Q_SIGNAL void isSpoilerChanged();
    Q_SIGNAL void spoilerHintChanged();
    Q_SIGNAL void isDraftChanged();

private:
    void setReply(Message &message, const QString &replyToJid, const QString &replyToGroupChatParticipantId, const QString &replyId, const QString &replyQuote);
    void loadDraft();
    void saveDraft();

    QString m_accountJid;
    QString m_chatJid;
    QString m_replaceId;
    QString m_replyToJid;
    QString m_replyToGroupChatParticipantId;
    QString m_replyToName;
    QString m_replyId;
    QString m_replyQuote;
    QString m_body;
    bool m_isSpoiler = false;
    QString m_spoilerHint;
    bool m_isDraft = false;

    FileSelectionModel *m_fileSelectionModel;
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
        PreviewImage,
        Type,
    };

    explicit FileSelectionModel(QObject *parent = nullptr);
    ~FileSelectionModel() override;

    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] int rowCount(const QModelIndex &parent) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

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

    QList<File> m_files;
};
