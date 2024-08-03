// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "DatabaseComponent.h"

class GroupChatUser;

class GroupChatUserDb : public DatabaseComponent
{
	Q_OBJECT

public:
	explicit GroupChatUserDb(Database *db, QObject *parent = nullptr);
	~GroupChatUserDb();

	static GroupChatUserDb *instance();

	/**
	 * Retrieves a user.
	 *
	 * @param accountJid JID of the account
	 * @param chatJid JID of the chat
	 * @param participantId ID of the participant
	 */
	QFuture<std::optional<GroupChatUser>> user(const QString &accountJid, const QString &chatJid, const QString &participantId);

	/**
	 * Retrieves a user.
	 *
	 * @param accountJid JID of the account
	 * @param chatJid JID of the chat
	 * @param participantId ID of the participant
	 */
	std::optional<GroupChatUser> _user(const QString &accountJid, const QString &chatJid, const QString &participantId);

	/**
	 * Retrieves users.
	 *
	 * @param accountJid JID of the account
	 * @param chatJid JID of the chat
	 * @param offset index of the first user being fetched (used for pagination, optional)
	 */
	QFuture<QVector<GroupChatUser>> users(const QString &accountJid, const QString &chatJid, const int offset = 0);

	/**
	 * Retrieves the bare JIDs of all group chat users.
	 *
	 * @param accountJid JID of the account
	 * @param chatJid JID of the chat
	 */
	QFuture<QVector<QString>> userJids(const QString &accountJid, const QString &chatJid);
	Q_SIGNAL void userJidsChanged(const QString &accountJid, const QString &chatJid);

	/**
	 * Handles a user that is allowed to participate in a specific group chat.
	 *
	 * @param user allowed user
	 */
	QFuture<void> handleUserAllowed(const GroupChatUser &user);

	/**
	 * Handles a user that is not allowed anymore to participate in a specific group chat or a user
	 * that is not banned anymore from participating in a specific group chat.
	 *
	 * @param user disallowed or unbanned user
	 */
	QFuture<void> handleUserDisallowedOrUnbanned(const GroupChatUser &user);

	/**
	 * Handles a received participant.
	 *
	 * That participant can be a new participant or an updated one.
	 * If it is a new participant, it will be added.
	 * Otherwise, it will update the existing one.
	 *
	 * @param participant new or updated participant
	 */
	QFuture<void> handleParticipantReceived(const GroupChatUser &participant);

	/**
	 * Handles a user who sent a message.
	 *
	 * That user can exist in the database or be a new user in case the user sent a message while
	 * the group was not joined yet.
	 * In the latter case, it is ensured that group chat user data can be processed for old messages
	 * even if their senders have not been stored after coming from the usual sources (e.g., the
	 * PubSub node for participants used by MIX channels).
	 *
	 * @param sender sender of a group chat message
	 */
	QFuture<void> handleMessageSender(GroupChatUser sender);

	/**
	 * Adds a user.
	 *
	 * @param user user being added
	 */
	QFuture<void> addUser(const GroupChatUser &user);
	Q_SIGNAL void userAdded(const GroupChatUser &user);

	Q_SIGNAL void userUpdated(const GroupChatUser &user);

	/**
	 * Removes a user.
	 *
	 * @param user user being removed
	 */
	QFuture<void> removeUser(const GroupChatUser &user);

	/**
	 * Removes all users of an account.
	 *
	 * @param accountJid JID of the account
	 */
	QFuture<void> removeUsers(const QString &accountJid);
	void _removeUsers(const QString &accountJid);

	/**
	 * Removes all users of a chat.
	 *
	 * @param accountJid JID of the account
	 * @param chatJid JID of the chat whose users are being removed
	 */
	QFuture<void> removeUsers(const QString &accountJid, const QString &chatJid);
	void _removeUsers(const QString &accountJid, const QString &chatJid);

private:
    /**
     * Updates a user by ID.
     *
     * @param accountJid JID of the account
     * @param chatJid JID of the chat
     * @param userId ID of the user
     * @param updateUser function being run to update the user
     */
    void updateUserById(const QString &accountJid, const QString &chatJid, const QString &userId, const std::function<void (GroupChatUser &user)> &updateUser);

    /**
     * Updates a user by JID.
     *
     * @param accountJid JID of the account
     * @param chatJid JID of the chat
     * @param userJid JID of the user
     * @param updateUser function being run to update the user
     */
    void updateUserByJid(const QString &accountJid, const QString &chatJid, const QString &userJid, const std::function<void (GroupChatUser &user)> &updateUser);

    /**
     * Updates a user by key-value pairs.
     *
     * @param updateUser function being run to update the user
     * @param keyValuePair key-value pairs for retrieving and updating the user
     */
    void updateUserByKeyValuePairs(const std::function<void (GroupChatUser &)> &updateUser, const QMap<QString, QVariant> &keyValuePairs);

	/**
	 * Parses users from a query.
	 *
	 * @param query query being run
	 * @param users users being parsed
	 */
	static void parseUsersFromQuery(QSqlQuery &query, QVector<GroupChatUser> &users);

	/**
	 * Creates a record for updating an old user by a new one.
	 *
	 * @param oldUser current user
	 * @param newUser user which updates the current one
	 */
	static QSqlRecord createUpdateRecord(const GroupChatUser &oldUser, const GroupChatUser &newUser);

	static GroupChatUserDb *s_instance;
};
