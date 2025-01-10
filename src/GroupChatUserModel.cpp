// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatUserModel.h"

#include "Algorithms.h"
#include "FutureUtils.h"
#include "Globals.h"
#include "GroupChatUserDb.h"
#include "RosterModel.h"

GroupChatUserModel::GroupChatUserModel(QObject *parent)
    : QAbstractListModel(parent)
{
    connect(GroupChatUserDb::instance(), &GroupChatUserDb::userAdded, this, &GroupChatUserModel::addUser);
    connect(GroupChatUserDb::instance(), &GroupChatUserDb::userUpdated, this, &GroupChatUserModel::updateUser);
    connect(GroupChatUserDb::instance(), &GroupChatUserDb::userRemoved, this, &GroupChatUserModel::removeUser);
}

int GroupChatUserModel::rowCount(const QModelIndex &) const
{
    return m_users.size();
}

QHash<int, QByteArray> GroupChatUserModel::roleNames() const
{
    return {{static_cast<int>(Role::Jid), QByteArrayLiteral("jid")},
            {static_cast<int>(Role::Name), QByteArrayLiteral("name")},
            {static_cast<int>(Role::Status), QByteArrayLiteral("status")},
            {static_cast<int>(Role::StatusText), QByteArrayLiteral("statusText")}};
}

QVariant GroupChatUserModel::data(const QModelIndex &index, int role) const
{
    if (!hasIndex(index.row(), index.column(), index.parent())) {
        qWarning() << "Could not get data from GroupChatUserModel." << index << role;
        return {};
    }

    const GroupChatUser &user = m_users.at(index.row());

    switch (static_cast<Role>(role)) {
    case Role::Jid:
        return user.jid;
    case Role::Name:
        return user.displayName();
    case Role::Status:
        return QVariant::fromValue(user.status);
    case Role::StatusText:
        switch (user.status) {
        case GroupChatUser::Status::Allowed:
            return tr("Not joined");
        case GroupChatUser::Status::Joined:
            return tr("Joined");
        case GroupChatUser::Status::Left:
            return tr("Left");
        case GroupChatUser::Status::Banned:
            return tr("Banned");
        }
    }

    return {};
}

QVariant GroupChatUserModel::data(const QModelIndex &index, Role role) const
{
    return data(index, static_cast<int>(role));
}

void GroupChatUserModel::fetchMore(const QModelIndex &)
{
    fetchUsers();
}

bool GroupChatUserModel::canFetchMore(const QModelIndex &) const
{
    return !m_fetchedAll;
}

QString GroupChatUserModel::accountJid() const
{
    return m_accountJid;
}

void GroupChatUserModel::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        Q_EMIT accountJidChanged();
    }
}

QString GroupChatUserModel::chatJid() const
{
    return m_chatJid;
}

void GroupChatUserModel::setChatJid(const QString &chatJid)
{
    if (m_chatJid != chatJid) {
        m_chatJid = chatJid;
        Q_EMIT chatJidChanged();
    }
}

QStringList GroupChatUserModel::userJids() const
{
    return transform<QStringList>(m_users, [](const GroupChatUser &user) {
        return user.jid;
    });
}

std::optional<const GroupChatUser> GroupChatUserModel::participant(const QString &accountJid, const QString &chatJid, const QString &participantId) const
{
    for (const auto &user : std::as_const(m_users)) {
        if (user.accountJid == accountJid && user.chatJid == chatJid && user.id == participantId) {
            return user;
        }
    }

    return std::nullopt;
}

QString GroupChatUserModel::participantName(const QString &accountJid, const QString &chatJid, const QString &participantId) const
{
    if (const auto user = participant(accountJid, chatJid, participantId)) {
        return user->displayName();
    }

    return {};
}

void GroupChatUserModel::fetchUsers()
{
    await(GroupChatUserDb::instance()->users(m_accountJid, m_chatJid, m_users.size()), this, [this](QList<GroupChatUser> &&users) {
        replaceUsers(users);

        if (users.size() < DB_QUERY_LIMIT_GROUP_CHAT_USERS) {
            m_fetchedAll = true;
        }
    });
}

void GroupChatUserModel::addUser(const GroupChatUser &user)
{
    if (!shouldUserBeProcessed(user)) {
        return;
    }

    for (int i = 0; i < m_users.size(); i++) {
        const GroupChatUser existingUser = m_users.at(i);

        if (user < existingUser) {
            insertUser(i, user);
            return;
        }
    }

    // Append the new user to the end of the list.
    insertUser(m_users.size(), user);
}

void GroupChatUserModel::replaceUsers(QList<GroupChatUser> users)
{
    std::sort(users.begin(), users.end());

    beginResetModel();
    m_users = filter(std::move(users), [this](const GroupChatUser &user) {
        return shouldUserBeProcessed(user);
    });
    endResetModel();

    Q_EMIT userJidsChanged();
}

void GroupChatUserModel::insertUser(int position, const GroupChatUser &user)
{
    beginInsertRows(QModelIndex(), position, position);
    m_users.insert(position, user);
    endInsertRows();

    Q_EMIT userJidsChanged();
}

void GroupChatUserModel::updateUser(const GroupChatUser &user)
{
    if (!shouldUserBeProcessed(user)) {
        return;
    }

    for (int i = 0; i < m_users.size(); i++) {
        if (m_users.at(i).accountJid == user.accountJid && m_users.at(i).chatJid == user.chatJid
            && (m_users.at(i).id == user.id || m_users.at(i).jid == user.jid)) {
            beginRemoveRows(QModelIndex(), i, i);
            m_users.removeAt(i);
            endRemoveRows();

            addUser(user);

            break;
        }
    }
}

void GroupChatUserModel::removeUser(const GroupChatUser &user)
{
    for (int i = 0; i < m_users.size(); i++) {
        GroupChatUser &existingUser = m_users[i];

        if (existingUser.accountJid == user.accountJid && existingUser.chatJid == user.chatJid && existingUser.id.isEmpty() ? existingUser.jid == user.jid
                                                                                                                            : existingUser.id == user.id) {
            beginRemoveRows(QModelIndex(), i, i);
            m_users.remove(i);
            endRemoveRows();

            Q_EMIT userJidsChanged();
            return;
        }
    }
}

void GroupChatUserModel::removeAllUsers()
{
    if (!m_users.isEmpty()) {
        beginRemoveRows(QModelIndex(), 0, rowCount() - 1);
        m_users.clear();
        endRemoveRows();
    }

    m_fetchedAll = false;
    Q_EMIT userJidsChanged();
}

bool GroupChatUserModel::shouldUserBeProcessed(const GroupChatUser &user)
{
    if (const auto rosterItem = RosterModel::instance()->findItem(m_chatJid)) {
        return rosterItem->groupChatParticipantId != user.id;
    }

    return false;
}

#include "moc_GroupChatUserModel.cpp"
