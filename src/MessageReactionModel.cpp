// SPDX-FileCopyrightText: 2014 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageReactionModel.h"

// Kaidan
#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
#include "KaidanCoreLog.h"

MessageReactionModel::MessageReactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageReactionModel::rowCount(const QModelIndex &parent) const
{
    return parent == QModelIndex() ? m_reactions.size() : 0;
}

QHash<int, QByteArray> MessageReactionModel::roleNames() const
{
    return {
        {static_cast<int>(Role::SenderJid), QByteArrayLiteral("senderJid")},
        {static_cast<int>(Role::SenderName), QByteArrayLiteral("senderName")},
        {static_cast<int>(Role::Emojis), QByteArrayLiteral("emojis")},
    };
}

QVariant MessageReactionModel::data(const QModelIndex &index, int role) const
{
    const auto row = index.row();

    if (!hasIndex(row, index.column(), index.parent())) {
        qCWarning(KAIDAN_CORE_LOG) << "Could not get data from MessageReactionModel." << index << role;
        return {};
    }

    const DetailedMessageReaction &reaction = m_reactions.at(row);

    switch (static_cast<Role>(role)) {
    case Role::SenderJid:
        return reaction.senderJid;
    case Role::SenderName:
        return reaction.senderName;
    case Role::Emojis:
        return reaction.emojis;
    }

    return {};
}

void MessageReactionModel::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        updateGroupChatUserData();
    }
}

void MessageReactionModel::setChatJid(const QString &chatJid)
{
    if (m_chatJid != chatJid) {
        m_chatJid = chatJid;
        updateGroupChatUserData();
    }
}

void MessageReactionModel::setReactions(const QList<DetailedMessageReaction> &reactions)
{
    if (m_reactions != reactions) {
        m_reactions = reactions;
        updateGroupChatUserData();
    }
}

void MessageReactionModel::updateGroupChatUserData()
{
    if (!m_accountJid.isEmpty() && !m_chatJid.isEmpty() && !m_reactions.isEmpty()) {
        for (auto &reaction : m_reactions) {
            GroupChatUserDb::instance()
                ->user(m_accountJid, m_chatJid, reaction.senderId)
                .then(this, [this, &reaction](const std::optional<GroupChatUser> user) {
                    beginResetModel();
                    if (user) {
                        reaction.senderJid = user->jid;
                        reaction.senderName = user->displayName();
                    }
                    endResetModel();
                });
        }
    }
}

#include "moc_MessageReactionModel.cpp"
