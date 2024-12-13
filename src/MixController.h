// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QObject>
#include <QSet>
// QXmpp
#include <QXmppMixManager.h>
#include <QXmppMixParticipantItem.h>
#include <QXmppStanza.h>
// Kaidan
#include "GroupChatController.h"

class QXmppMixInfoItem;

/**
 * This class provides XEP-0369: Mediated Information eXchange (MIX) functionality.
 */
class MixController : public QObject
{
    Q_OBJECT

public:
    MixController(QObject *parent = nullptr);

    bool channelParticipationSupported() const;
    QList<GroupChatService> groupChatServices() const;

    void createPrivateChannel(const QString &serviceJid);
    void createPublicChannel(const QString &channelJid);

    void requestChannelAccessibility(const QString &channelJid);
    void requestChannelInformation(const QString &channelJid);

    void renameChannel(const QString &channelJid, const QString &newChannelName);
    void joinChannel(const QString &channelJid, const QString &nickname);

    void inviteContactToChannel(const QString &channelJid, const QString &contactJid, bool channelPublic);

    void requestChannelUsers(const QString &channelJid);
    void banUser(const QString &channelJid, const QString &userJid);

    void leaveChannel(const QString &channelJid);
    void deleteChannel(const QString &channelJid);

private:
    void handleParticipantSupportChanged();
    void handleServicesChanged();
    void handleChannelInformationUpdated(const QString &channelJid, const QXmppMixInfoItem &information);

    void handleChannelAccessibilityReceived(const QString &channelJid, const bool isPublic);
    void handleChannelMadePrivate(const QString &channelJid);

    void handleJidAllowed(const QString &channelJid, const QString &jid);
    void handleJidDisallowed(const QString &channelJid, const QString &jid);

    void handleJidBanned(const QString &channelJid, const QString &jid);
    void handleJidUnbanned(const QString &channelJid, const QString &jid);

    void handleParticipantReceived(const QString &channelJid, const QXmppMixParticipantItem &participantItem);
    void handleParticipantLeft(const QString &channelJid, const QString &participantId);

    /**
     * Called after a channel is deleted.
     *
     * If the channel was deleted by the user, the leaving of the channel is triggered
     * to let the server remove the corresponding roster item.
     * Otherwise, the signal channelDeleted() is emitted.
     *
     * @param channelJid JID of the left channel
     */
    void handleChannelDeleted(const QString &channelJid);

    void handleChannelDeletionFailed(const QString &channelJid, const QXmppStanza::Error &error);

    bool m_channelParticipationSupported = false;
    QList<QXmppMixManager::Service> m_services;
};
