// SPDX-FileCopyrightText: 2014 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MessageReactionModel.h"

#include "GroupChatUser.h"
#include "GroupChatUserDb.h"
// #include "MessageDb.h"
// #include "RosterModel.h"

MessageReactionModel::MessageReactionModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int MessageReactionModel::rowCount(const QModelIndex &) const
{
    return reactions.size();
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
        qWarning() << "Could not get data from MessageReactionModel." << index << role;
        return {};
    }

    const DetailedMessageReaction &reaction = reactions.at(row);

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

void MessageReactionModel::setReactions(const QList<DetailedMessageReaction> &reactions)
{
    if (this->reactions != reactions) {
        this->reactions = reactions;

        for (auto &reaction : this->reactions) {
            await(GroupChatUserDb::instance()->user(accountJid, chatJid, reaction.senderId), this, [this, &reaction](const std::optional<GroupChatUser> user) {
                if (user) {
                    beginResetModel();
                    reaction.senderJid = user->jid;
                    reaction.senderName = user->displayName();
                    endResetModel();
                }
            });
        }
    }
}

#include "moc_MessageReactionModel.cpp"
