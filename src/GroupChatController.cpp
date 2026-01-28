// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatController.h"

// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "ChatController.h"
#include "GroupChatUserDb.h"
#include "MainController.h"
#include "MixController.h"
#include "RosterDb.h"
#include "RosterModel.h"

GroupChatController::GroupChatController(AccountSettings *accountSettings, MessageController *messageController, QXmppMixManager *mixManager, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_mixController(new MixController(accountSettings, this, messageController, mixManager, this))
{
    connect(RosterDb::instance(), &RosterDb::itemAdded, this, &GroupChatController::requestGroupChatData);
    connect(RosterModel::instance(), &RosterModel::itemAdded, this, &GroupChatController::handleRosterItemAdded);

    connect(this, &GroupChatController::groupChatMadePrivate, this, [this](const QString &groupChatJid) {
        RosterDb::instance()->updateItem(m_accountSettings->jid(), groupChatJid, [](RosterItem &item) {
            item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public, false);
        });
    });

    connect(this, &GroupChatController::groupChatMadePublic, this, [this](const QString &groupChatJid) {
        RosterDb::instance()->updateItem(m_accountSettings->jid(), groupChatJid, [](RosterItem &item) {
            item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public);
        });
    });

    connect(this, &GroupChatController::userAllowedOrBanned, GroupChatUserDb::instance(), &GroupChatUserDb::handleUserAllowedOrBanned);
    connect(this, &GroupChatController::userDisallowedOrUnbanned, GroupChatUserDb::instance(), &GroupChatUserDb::handleUserDisallowedOrUnbanned);

    connect(this, &GroupChatController::participantReceived, GroupChatUserDb::instance(), &GroupChatUserDb::handleParticipantReceived);
    connect(this, &GroupChatController::participantLeft, GroupChatUserDb::instance(), &GroupChatUserDb::handleParticipantLeft);

    connect(this, &GroupChatController::groupChatLeft, GroupChatUserDb::instance(), qOverload<const QString &>(&GroupChatUserDb::removeUsers));

    connect(this, &GroupChatController::groupChatDeleted, this, [this](const QString &groupChatJid) {
        RosterDb::instance()->updateItem(m_accountSettings->jid(), groupChatJid, [](RosterItem &item) {
            item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Deleted);
        });
    });

    auto setIdle = [this]() {
        setBusy(false);
    };

    connect(this, &GroupChatController::groupChatCreated, this, setIdle);
    connect(this, &GroupChatController::privateGroupChatCreationFailed, this, setIdle);
    connect(this, &GroupChatController::publicGroupChatCreationFailed, this, setIdle);

    // groupChatJoined() is not covered here since successful joining is handled by
    // handleRosterItemAdded().
    connect(this, &GroupChatController::groupChatJoiningFailed, this, setIdle);

    connect(this, &GroupChatController::groupChatLeft, this, setIdle);
    connect(this, &GroupChatController::groupChatLeavingFailed, this, setIdle);

    connect(this, &GroupChatController::groupChatDeleted, this, setIdle);
    connect(this, &GroupChatController::groupChatDeletionFailed, this, setIdle);
}

bool GroupChatController::busy()
{
    return m_busy;
}

bool GroupChatController::groupChatParticipationSupported() const
{
    return m_mixController->channelParticipationSupported();
}

bool GroupChatController::groupChatCreationSupported() const
{
    return std::ranges::any_of(m_mixController->groupChatServices(), [](const GroupChatService &service) {
        return service.groupChatCreationSupported;
    });
}

QStringList GroupChatController::groupChatServiceJids() const
{
    return transformFilter<QStringList>(m_mixController->groupChatServices(), [](const GroupChatService &service) -> std::optional<QString> {
        return service.jid;
    });
}

void GroupChatController::createPrivateGroupChat(const QString &serviceJid)
{
    setBusy(true);
    m_mixController->createPrivateChannel(serviceJid);
}

void GroupChatController::createPublicGroupChat(const QString &groupChatJid)
{
    setBusy(true);
    m_mixController->createPublicChannel(groupChatJid);
}

void GroupChatController::requestGroupChatAccessibility(const QString &groupChatJid)
{
    m_mixController->requestChannelAccessibility(groupChatJid);
}

void GroupChatController::requestGroupChatInformation(const QString &groupChatJid)
{
    m_mixController->requestChannelInformation(groupChatJid);
}

void GroupChatController::renameGroupChat(const QString &groupChatJid, const QString &name)
{
    m_mixController->renameChannel(groupChatJid, name);
}

void GroupChatController::joinGroupChat(const QString &groupChatJid, const QString &nickname)
{
    setBusy(true);
    m_processedGroupChatJid = groupChatJid;
    m_mixController->joinChannel(groupChatJid, nickname);
}

void GroupChatController::inviteContactToGroupChat(const QString &groupChatJid, const QString &contactJid, bool groupChatPublic)
{
    m_mixController->inviteContactToChannel(groupChatJid, contactJid, groupChatPublic);
}

void GroupChatController::requestGroupChatUsers(const QString &groupChatJid)
{
    m_mixController->requestChannelUsers(groupChatJid);
}

void GroupChatController::banUser(const QString &groupChatJid, const QString &userJid)
{
    m_mixController->banUser(groupChatJid, userJid);
}

void GroupChatController::leaveGroupChat(const QString &groupChatJid)
{
    setBusy(true);
    m_mixController->leaveChannel(groupChatJid);
}

void GroupChatController::deleteGroupChat(const QString &groupChatJid)
{
    setBusy(true);
    m_mixController->deleteChannel(groupChatJid);
}

void GroupChatController::requestGroupChatData(const RosterItem &rosterItem)
{
    // Requesting the group chat's data is done here and not within the joining method of the group chat controller to cover both cases:
    //   1. This client joined the group chat (could be done during joining).
    //   2. Another own client joined the group chat (must be covered here).
    if (rosterItem.accountJid == m_accountSettings->jid() && rosterItem.isGroupChat()) {
        requestGroupChatAccessibility(rosterItem.jid);
        requestGroupChatInformation(rosterItem.jid);
    }
}

void GroupChatController::handleRosterItemAdded(const RosterItem &rosterItem)
{
    const auto accountJid = rosterItem.accountJid;
    const auto jid = rosterItem.jid;

    if (accountJid == m_accountSettings->jid() && jid == m_processedGroupChatJid) {
        m_processedGroupChatJid.clear();
        Q_EMIT MainController::instance()->openChatPageRequested(accountJid, jid);
        setBusy(false);
    }
}

void GroupChatController::setBusy(bool busy)
{
    if (m_busy != busy) {
        m_busy = busy;
        Q_EMIT busyChanged();
    }
}

#include "moc_GroupChatController.cpp"
