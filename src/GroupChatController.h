// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

class GroupChatUser;
class MixController;

struct GroupChatService
{
	QString accountJid;
	QString jid;
	bool groupChatsSearchable = false;
	bool groupChatCreationSupported = false;
};

/**
 * This class controlls the group chat functionality.
 */
class GroupChatController : public QObject
{
	Q_OBJECT

	Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
	Q_PROPERTY(bool groupChatParticipationSupported READ groupChatParticipationSupported NOTIFY groupChatParticipationSupportedChanged)
	Q_PROPERTY(bool groupChatCreationSupported READ groupChatCreationSupported NOTIFY groupChatServicesChanged)

public:
	static GroupChatController *instance();

	GroupChatController(QObject *parent = nullptr);
	~GroupChatController();

	bool busy();
	Q_SIGNAL void busyChanged();

	bool groupChatParticipationSupported() const;
	Q_SIGNAL void groupChatParticipationSupportedChanged();
	bool groupChatCreationSupported() const;
	Q_INVOKABLE QStringList groupChatServiceJids(const QString &accountJid) const;
	Q_SIGNAL void groupChatServicesChanged();

	Q_INVOKABLE void createPrivateGroupChat(const QString &accountJid, const QString &serviceJid);
	Q_SIGNAL void privateGroupChatCreationFailed(const QString &serviceJid, const QString &errorMessage);

	Q_INVOKABLE void createPublicGroupChat(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void publicGroupChatCreationFailed(const QString &groupChatJid, const QString &errorMessage);

	Q_SIGNAL void groupChatCreated(const QString &accountJid, const QString &groupChatJid);

	void requestGroupChatAccessibility(const QString &accountJid, const QString &groupChatJid);
	void requestChannelInformation(const QString &accountJid, const QString &channelJid);

	Q_INVOKABLE void renameGroupChat(const QString &accountJid, const QString &groupChatJid, const QString &name);

	Q_INVOKABLE void joinGroupChat(const QString &accountJid, const QString &groupChatJid, const QString &nickname);
	Q_SIGNAL void groupChatJoined(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatJoiningFailed(const QString &groupChatJid, const QString &errorMessage);

	Q_INVOKABLE void inviteContactToGroupChat(const QString &accountJid, const QString &groupChatJid, const QString &contactJid, bool groupChatPublic);
	Q_SIGNAL void contactInvitedToGroupChat(const QString &accountJid, const QString &groupChatJid, const QString &inviteeJid);
	Q_SIGNAL void groupChatInviteeSelectionNeeded();

	void requestGroupChatUsers(const QString &accountJid, const QString &groupChatJid);
	QVector<QString> currentUserJids() const;
	Q_SIGNAL void currentUserJidsChanged();

	Q_INVOKABLE void banUser(const QString &accountJid, const QString &groupChatJid, const QString &userJid);
	Q_SIGNAL void userBanned(const GroupChatUser &user);
	Q_SIGNAL void userUnbanned(const GroupChatUser &user);

	Q_INVOKABLE void leaveGroupChat(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatLeft(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatLeavingFailed(const QString &accountJid, const QString &groupChatJid, const QString &errorMessage);

	Q_INVOKABLE void deleteGroupChat(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatDeleted(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatDeletionFailed(const QString &groupChatJid, const QString &errorMessage);

	Q_SIGNAL void groupChatMadePrivate(const QString &accountJid, const QString &groupChatJid);
	Q_SIGNAL void groupChatMadePublic(const QString &accountJid, const QString &groupChatJid);

	Q_SIGNAL void userAllowed(const GroupChatUser &user);
	Q_SIGNAL void userDisallowed(const GroupChatUser &user);

	Q_SIGNAL void participantReceived(const GroupChatUser &participant);
	Q_SIGNAL void participantLeft(const GroupChatUser &participant);

	Q_SIGNAL void removeGroupChatUsersRequested(const QString &accountJid);

private:
	void setBusy(bool busy);
	void handleChatChanged();
	void updateUserJidsChanged(const QString &accountJid, const QString &groupChatJid);
	void updateEncryption();
	void setCurrentUserJids(const QVector<QString> &currentUserJids);

	bool m_busy = false;
	std::unique_ptr<MixController> m_mixController;
	QVector<QString> m_currentUserJids;

	static GroupChatController *s_instance;
};
