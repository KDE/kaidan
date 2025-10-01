// SPDX-FileCopyrightText: 2014 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QAbstractListModel>
// Kaidan
#include "MessageModel.h"

class MessageReactionModel : public QAbstractListModel
{
    Q_OBJECT
    Q_PROPERTY(QString accountJid MEMBER m_accountJid WRITE setAccountJid)
    Q_PROPERTY(QString chatJid MEMBER m_chatJid WRITE setChatJid)
    Q_PROPERTY(QList<DetailedMessageReaction> reactions MEMBER m_reactions WRITE setReactions)

public:
    enum class Role {
        SenderJid = Qt::UserRole + 1,
        SenderName,
        Emojis,
    };

    explicit MessageReactionModel(QObject *parent = nullptr);
    ~MessageReactionModel() override = default;

    [[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;

    void setAccountJid(const QString &accountJid);
    void setChatJid(const QString &chatJid);
    void setReactions(const QList<DetailedMessageReaction> &reactions);

private:
    void updateGroupChatUserData();

    QString m_accountJid;
    QString m_chatJid;
    QList<DetailedMessageReaction> m_reactions;
};
