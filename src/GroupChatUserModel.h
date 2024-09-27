// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// std
#include <optional>
// Qt
#include <QAbstractListModel>
#include <QStringList>
// Kaidan
#include "GroupChatUser.h"

class GroupChatUserModel : public QAbstractListModel
{
	Q_OBJECT

	Q_PROPERTY(QString accountJid READ accountJid WRITE setAccountJid NOTIFY accountJidChanged)
	Q_PROPERTY(QString chatJid READ chatJid WRITE setChatJid NOTIFY chatJidChanged)

public:
	enum class Role {
		Jid = Qt::UserRole + 1,
		Name,
		Status,
		StatusText
	};
	Q_ENUM(Role)

	GroupChatUserModel(QObject *parent = nullptr);

	[[nodiscard]] int rowCount(const QModelIndex &parent = QModelIndex()) const override;
	[[nodiscard]] QHash<int, QByteArray> roleNames() const override;
	[[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
	QVariant data(const QModelIndex &index, GroupChatUserModel::Role role) const;

	Q_INVOKABLE void fetchMore(const QModelIndex &parent) override;
	Q_INVOKABLE bool canFetchMore(const QModelIndex &parent) const override;

	QString accountJid() const;
	void setAccountJid(const QString &accountJid);
	Q_SIGNAL void accountJidChanged();

	QString chatJid() const;
	void setChatJid(const QString &chatJid);
	Q_SIGNAL void chatJidChanged();

	QStringList userJids() const;
	Q_SIGNAL void userJidsChanged();

	/**
	 * Searches a participant by a given ID.
	 *
	 * @param accountJid JID of the account that joined the group chat
	 * @param chatJid JID of the group chat whose participant is searched
	 * @param participantId ID of the participant
	 *
	 * @return the found participant or a null pointer if no participant with the given ID could be
	 *         found
	 */
	std::optional<const GroupChatUser> participant(const QString &accountJid, const QString &chatJid, const QString &participantId) const;

	/**
	 * Provides a suitable group chat participant name.
	 *
	 * The participant name is determined as follows:
	 *
	 *  1. If no participant with the ID can be found, an empty string is returned.
	 *  2. If the participant is in the roster and the roster item has a name, that name is returned.
	 *  3. If the group chat provides a participant name, that name is returned.
	 *  4. If the group chat provides a  participant JID, that JID is returned.
	 *  5. If 2, 3 and 4 are not applicable, the participant ID is returned.
	 *
	 * @param accountJid JID of the account whose group chat participant's name is determined
	 * @param chatJid JID of the group chat whose participant's name is determined
	 * @param participantId ID of the group chat participant
	 *
	 * @return a suitable name for the group chat participant
	 */
	Q_INVOKABLE QString participantName(const QString &accountJid, const QString &chatJid, const QString &participantId) const;

private:
	void fetchUsers();

	/**
	 * Adds a new user.
	 *
	 * If the passed user is the user's own user entry, it is not added.
	 *
	 * @param user new user to add
	 */
	void addUser(const GroupChatUser &user);

	/**
	 * Replaces all users.
	 *
	 * The user's own user entry is not added.
	 *
	 * @param users new and already sorted users to add
	 */
	void replaceUsers(QVector<GroupChatUser> users);

	void insertUser(int i, const GroupChatUser &user);
	void updateUser(const GroupChatUser &user);
	void removeUser(const GroupChatUser &user);
	void removeAllUsers();

	/**
	 * Checks if a user should be processed like being added or updated.
	 *
	 * @param user user being checked
	 *
	 * @return true if the passed user should be processed, otherwise false
	 */
	bool shouldUserBeProcessed(const GroupChatUser &user);

	/**
	 * Determines a suitable name for a user.
	 *
	 * The user name is determined as follows:
	 *
	 *  1. If the user is in the roster and the roster item has a name, that name is returned.
	 *  2. If the group chat provides a name for the user, that name is returned.
	 *  3. If the group chat provides a JID for the user, that JID is returned.
	 *  4. If 1, 2 and 3 are not applicable, the user's ID is returned.
	 *
	 * @param user user for whom a suitable name is determined
	 *
	 * @return a suitable name for the user
	 */
	QString determineUserName(const GroupChatUser &user) const;

	QString m_accountJid;
	QString m_chatJid;
	QVector<GroupChatUser> m_users;
	bool m_fetchedAll = false;
};
