// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
// Kaidan
#include "Encryption.h"

struct GroupChatUser;
struct RosterItem;

class AccountSettings;
class MessageController;
class MixController;
class QXmppMixManager;

struct GroupChatService {
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
    GroupChatController(AccountSettings *accountSettings, MessageController *messageController, QXmppMixManager *mixManager, QObject *parent = nullptr);

    bool busy();
    Q_SIGNAL void busyChanged();

    bool groupChatParticipationSupported() const;
    Q_SIGNAL void groupChatParticipationSupportedChanged();
    bool groupChatCreationSupported() const;
    Q_INVOKABLE QStringList groupChatServiceJids() const;
    Q_SIGNAL void groupChatServicesChanged();

    Q_INVOKABLE void createPrivateGroupChat(const QString &serviceJid);
    Q_SIGNAL void privateGroupChatCreationFailed(const QString &serviceJid, const QString &errorMessage);

    Q_INVOKABLE void createPublicGroupChat(const QString &groupChatJid);
    Q_SIGNAL void publicGroupChatCreationFailed(const QString &groupChatJid, const QString &errorMessage);

    Q_SIGNAL void groupChatCreated(const QString &groupChatJid);

    void requestGroupChatAccessibility(const QString &groupChatJid);
    void requestGroupChatInformation(const QString &groupChatJid);

    Q_INVOKABLE void renameGroupChat(const QString &groupChatJid, const QString &name);

    Q_INVOKABLE void joinGroupChat(const QString &groupChatJid, const QString &nickname);
    Q_SIGNAL void groupChatJoined(const QString &groupChatJid);
    Q_SIGNAL void groupChatJoiningFailed(const QString &groupChatJid, const QString &errorMessage);

    Q_INVOKABLE void inviteContactToGroupChat(const QString &groupChatJid, const QString &contactJid, bool groupChatPublic);
    Q_SIGNAL void contactInvitedToGroupChat(const QString &groupChatJid, const QString &inviteeJid);
    Q_SIGNAL void groupChatInviteeSelectionNeeded();

    void requestGroupChatUsers(const QString &groupChatJid);

    Q_INVOKABLE void leaveGroupChat(const QString &groupChatJid);
    Q_SIGNAL void groupChatLeft(const QString &groupChatJid);
    Q_SIGNAL void groupChatLeavingFailed(const QString &groupChatJid, const QString &errorMessage);

    Q_INVOKABLE void deleteGroupChat(const QString &groupChatJid);
    Q_SIGNAL void groupChatDeleted(const QString &groupChatJid);
    Q_SIGNAL void groupChatDeletionFailed(const QString &groupChatJid, const QString &errorMessage);

    Q_SIGNAL void groupChatMadePrivate(const QString &groupChatJid);
    Q_SIGNAL void groupChatMadePublic(const QString &groupChatJid);

    Q_INVOKABLE void banUser(const QString &groupChatJid, const QString &userJid);

    Q_SIGNAL void userDisallowedOrUnbanned(const GroupChatUser &user);
    Q_SIGNAL void userAllowedOrBanned(const GroupChatUser &user);

    Q_SIGNAL void participantReceived(const GroupChatUser &participant);
    Q_SIGNAL void participantLeft(const GroupChatUser &participant);

private:
    void requestGroupChatData(const RosterItem &rosterItem);
    void handleRosterItemAdded(const RosterItem &rosterItem);
    void setBusy(bool busy);

    AccountSettings *const m_accountSettings;
    MixController *const m_mixController;

    QString m_processedGroupChatJid;
    bool m_busy = false;
};
