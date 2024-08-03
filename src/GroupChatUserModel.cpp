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
}

int GroupChatUserModel::rowCount(const QModelIndex&) const
{
	return m_users.size();
}

QHash<int, QByteArray> GroupChatUserModel::roleNames() const
{
	return {
		{ static_cast<int>(Role::Jid), "jid" },
		{ static_cast<int>(Role::Name), "name" },
		{ static_cast<int>(Role::Status), "status" },
		{ static_cast<int>(Role::StatusText), "statusText" }
	};
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
	QStringList userJids;

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

void GroupChatUserModel::handleUserAllowed(const GroupChatUser &user)
{
	if (shouldUserBeProcessed(user)) {
		for (const GroupChatUser &existingUser : std::as_const(m_users)) {
			if (existingUser.jid == user.jid) {
				return;
			}
		}

		addUser(user);
	}
}

void GroupChatUserModel::handleBannedUserAdded(const GroupChatUser &user)
{
	if (shouldUserBeProcessed(user)) {
		addUser(user);
	}
}

void GroupChatUserModel::handleUserDisallowedOrUnbanned(const GroupChatUser &user)
{
	if (shouldUserBeProcessed(user)) {
		for (const GroupChatUser &existingUser : std::as_const(m_users)) {
			if (existingUser.jid == user.jid && existingUser.status == user.status) {
				removeUser(user);
				return;
			}
		}
	}
}

void GroupChatUserModel::handleParticipantReceived(const GroupChatUser &participant)
{
	if (shouldUserBeProcessed(participant)) {
		for (const GroupChatUser &existingUser : std::as_const(m_users)) {
			if (existingUser.accountJid == participant.accountJid && existingUser.chatJid == participant.chatJid) {
				// If the participant was allowed to join but not yet joined, transform the former entry to a joined one.
				// If the participant was already joined but modified, update the former entry normally.
				if (existingUser.status != GroupChatUser::Status::Joined && existingUser.jid == participant.jid) {
					updateUser(participant.accountJid, participant.chatJid, participant.jid, [=] (GroupChatUser &user) {
						user.id = participant.id;
						user.jid = participant.jid;
						user.name = participant.name;
						user.status = participant.status;
					}, false);

					return;
				} else if (existingUser.id == participant.id) {
					updateUser(participant.accountJid, participant.chatJid, participant.id, [=] (GroupChatUser &user) {
						user.name = participant.name;
					});
					return;
				}

			}
		}

		// If the participant was not set as allowed before and joined now, simply add the participant.
		addUser(participant);
	}
}

void GroupChatUserModel::removeUser(const GroupChatUser &user)
{
	for (int i = 0; i < m_users.size(); i++) {
		GroupChatUser &existingUser = m_users[i];

		// The existence of the user ID must be checked because users can have only an ID (participant in anonymous group chat), only a JID (not joined member) or both (participant in normal group chat).
		if (existingUser.accountJid == user.accountJid && existingUser.chatJid == user.chatJid && !existingUser.id.isEmpty() ? existingUser.id == user.id : existingUser.jid == user.jid) {
			beginRemoveRows(QModelIndex(), i, i);
			m_users.remove(i);
			endRemoveRows();

			Q_EMIT userJidsChanged();
			return;
		}
	}
}

void GroupChatUserModel::fetchUsers()
{
	await(GroupChatUserDb::instance()->users(m_accountJid, m_chatJid, m_users.size()), this, [this](QVector<GroupChatUser> &&users) {
		addSortedUsers(users);

		if (users.size() < DB_QUERY_LIMIT_GROUP_CHAT_USERS) {
			m_fetchedAll = true;
		}
	});
}

void GroupChatUserModel::addUser(const GroupChatUser &user)
{
	for (int i = 0; i < m_users.size(); i++) {
		const GroupChatUser existingUser = m_users.at(i);

		if (user.name > existingUser.name) {
			insertUser(i, user);
			return;
		}
	}

	// Append the new user to the end of the list.
	insertUser(m_users.size(), user);
}

void GroupChatUserModel::addSortedUsers(const QVector<GroupChatUser> &users)
{
	for (const GroupChatUser &user : users) {
		int i = 0;

		if (shouldUserBeProcessed(user)) {
			insertUser(i, user);
			i++;
		}
	}
}

void GroupChatUserModel::insertUser(int position, const GroupChatUser &user)
{
	beginInsertRows(QModelIndex(), position, position);
	m_users.insert(position, user);
	endInsertRows();

	Q_EMIT userJidsChanged();
}

void GroupChatUserModel::updateUser(const QString &accountJid, const QString &chatJid, const QString &idOrJid, const std::function<void (GroupChatUser &user)> &updateUser, const bool isIdPassed)
{
	for (int i = 0; i < m_users.size(); i++) {
		if (m_users.at(i).accountJid == accountJid && m_users.at(i).chatJid == chatJid && (isIdPassed ? m_users.at(i).id == idOrJid : m_users.at(i).jid == idOrJid)) {
			// Update the user.
			GroupChatUser user = m_users.at(i);
			updateUser(user);

			// Return if the user was not modified.
			if (m_users.at(i) == user) {
				return;
			}

			// Check if the position of the new user is different.
			// Insert the user at the same position if it is equal.
			// Otherwise, remove the old user and append the new one to the end.
			if (user.name == m_users.at(i).name) {
				beginRemoveRows(QModelIndex(), i, i);
				m_users.removeAt(i);
				endRemoveRows();

				insertUser(i, user);
			} else {
				beginRemoveRows(QModelIndex(), i, i);
				m_users.removeAt(i);
				endRemoveRows();

				addUser(user);
			}
			break;
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
