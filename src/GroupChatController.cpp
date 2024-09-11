// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatController.h"

// Kaidan
#include "Algorithms.h"
#include "ChatController.h"
#include "EncryptionController.h"
#include "EncryptionWatcher.h"
#include "GroupChatUserDb.h"
//#include "GroupChatUserModel.h"
#include "Kaidan.h"
#include "MixController.h"
#include "RosterDb.h"
#include "RosterModel.h"

GroupChatController *GroupChatController::s_instance = nullptr;

GroupChatController *GroupChatController::instance()
{
	return s_instance;
}

GroupChatController::GroupChatController(QObject *parent)
	: QObject(parent), m_mixController(std::make_unique<MixController>())
{
	Q_ASSERT(!GroupChatController::s_instance);
	s_instance = this;

	connect(GroupChatUserDb::instance(), &GroupChatUserDb::userJidsChanged, this, &GroupChatController::updateUserJidsChanged);
	connect(ChatController::instance(), &ChatController::chatChanged, this, &GroupChatController::handleChatChanged);

	connect(this, &GroupChatController::groupChatMadePrivate, this, [](const QString &, const QString &groupChatJid) {
		RosterDb::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public, false);
		});

		RosterModel::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public, false);
		});
	});

	connect(this, &GroupChatController::groupChatMadePublic, this, [](const QString &, const QString &groupChatJid) {
		RosterDb::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public);
		});

		RosterModel::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Public);
		});
	});

	connect(this, &GroupChatController::userAllowed, GroupChatUserDb::instance(), &GroupChatUserDb::handleUserAllowed);
	connect(this, &GroupChatController::userDisallowed, GroupChatUserDb::instance(), &GroupChatUserDb::handleUserDisallowedOrUnbanned);

	connect(this, &GroupChatController::userBanned, GroupChatUserDb::instance(), &GroupChatUserDb::addUser);
	connect(this, &GroupChatController::userUnbanned, GroupChatUserDb::instance(), &GroupChatUserDb::handleUserDisallowedOrUnbanned);

	connect(this, &GroupChatController::participantReceived, GroupChatUserDb::instance(), &GroupChatUserDb::handleParticipantReceived);
	connect(this, &GroupChatController::participantLeft, GroupChatUserDb::instance(), &GroupChatUserDb::removeUser);

	connect(this, &GroupChatController::groupChatLeft, GroupChatUserDb::instance(), qOverload<const QString &, const QString &>(&GroupChatUserDb::removeUsers));

	connect(this, &GroupChatController::groupChatDeleted, this, [](const QString &, const QString &groupChatJid) {
		RosterDb::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Deleted);
		});

		RosterModel::instance()->updateItem(groupChatJid, [](RosterItem &item) {
			item.groupChatFlags = item.groupChatFlags.setFlag(RosterItem::GroupChatFlag::Deleted);
		});
	});

	auto setIdle = [this]() {
		setBusy(false);
	};

	connect(this, &GroupChatController::groupChatCreated, this, setIdle);
	connect(this, &GroupChatController::privateGroupChatCreationFailed, this, setIdle);
	connect(this, &GroupChatController::publicGroupChatCreationFailed, this, setIdle);

	connect(this, &GroupChatController::groupChatJoined, this, setIdle);
	connect(this, &GroupChatController::groupChatJoiningFailed, this, setIdle);

	connect(this, &GroupChatController::groupChatLeft, this, setIdle);
	connect(this, &GroupChatController::groupChatLeavingFailed, this, setIdle);

	connect(this, &GroupChatController::groupChatDeleted, this, setIdle);
	connect(this, &GroupChatController::groupChatDeletionFailed, this, setIdle);
}

GroupChatController::~GroupChatController()
{
	s_instance = nullptr;
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
	const auto groupChatServices = m_mixController->groupChatServices();
	return std::any_of(groupChatServices.cbegin(), groupChatServices.cend(), [](const GroupChatService &service) {
		return service.groupChatCreationSupported;
	});
}

QStringList GroupChatController::groupChatServiceJids(const QString &accountJid) const
{
	return transformFilter<QStringList>(m_mixController->groupChatServices(), [accountJid](const GroupChatService &service) -> std::optional<QString> {
		if (service.accountJid == accountJid) {
			return service.jid;
		}

		return {};
	});
}

void GroupChatController::createPrivateGroupChat(const QString &, const QString &serviceJid)
{
	setBusy(true);
	m_mixController->createPrivateChannel(serviceJid);
}

void GroupChatController::createPublicGroupChat(const QString &, const QString &groupChatJid)
{
	setBusy(true);
	m_mixController->createPublicChannel(groupChatJid);
}

void GroupChatController::requestGroupChatAccessibility(const QString &, const QString &groupChatJid)
{
	m_mixController->requestChannelAccessibility(groupChatJid);
}

void GroupChatController::requestChannelInformation(const QString &, const QString &channelJid)
{
	m_mixController->requestChannelInformation(channelJid);
}

void GroupChatController::renameGroupChat(const QString &, const QString &groupChatJid, const QString &name)
{
	m_mixController->renameChannel(groupChatJid, name);
}

void GroupChatController::joinGroupChat(const QString &, const QString &groupChatJid, const QString &nickname)
{
	setBusy(true);
	m_mixController->joinChannel(groupChatJid, nickname);
}

void GroupChatController::inviteContactToGroupChat(const QString &, const QString &groupChatJid, const QString &contactJid, bool groupChatPublic)
{
	m_mixController->inviteContactToChannel(groupChatJid, contactJid, groupChatPublic);
}

void GroupChatController::requestGroupChatUsers(const QString &, const QString &groupChatJid)
{
	m_mixController->requestChannelUsers(groupChatJid);
}

QVector<QString> GroupChatController::currentUserJids() const
{
	return m_currentUserJids;
}

void GroupChatController::banUser(const QString &, const QString &groupChatJid, const QString &userJid)
{
	m_mixController->banUser(groupChatJid, userJid);
}

void GroupChatController::leaveGroupChat(const QString &, const QString &groupChatJid)
{
	setBusy(true);
	m_mixController->leaveChannel(groupChatJid);
}

void GroupChatController::deleteGroupChat(const QString &, const QString &groupChatJid)
{
	setBusy(true);
	m_mixController->deleteChannel(groupChatJid);
}

void GroupChatController::setBusy(bool busy)
{
	if (m_busy != busy) {
		m_busy = busy;
		Q_EMIT busyChanged();
	}
}

void GroupChatController::handleChatChanged()
{
	if (auto chatController = ChatController::instance(); chatController->rosterItem().isGroupChat()) {
		const auto accountJid = chatController->accountJid();
		const auto chatJid = chatController->chatJid();

		await(GroupChatUserDb::instance()->userJids(accountJid, chatJid), this, [this, chatController, accountJid, chatJid](QVector<QString> &&userJids) {
			// Ensure that the chat is still the open one after fetching the user JIDs.
			if (chatController->accountJid() == accountJid && chatController->chatJid() == chatJid) {
				setCurrentUserJids(userJids);

				// Handle the case when the database does not contain users for the current group chat.
				// That happens, for example, after joining a channel or when the database was manually
				// deleted.
				//
				// The encryption for group chats is initialized once all group chat user JIDs are
				// fetched.
				if (m_currentUserJids.contains(accountJid)) {
					updateEncryption();
				} else {
					requestGroupChatUsers(accountJid, chatJid);
				}
			}
		});
	}
}

void GroupChatController::updateUserJidsChanged(const QString &accountJid, const QString &groupChatJid)
{
	auto chatController = ChatController::instance();
	const auto currentAccountJid = chatController->accountJid();
	const auto currentChatJid = chatController->chatJid();

	if (currentAccountJid == accountJid && currentChatJid == groupChatJid) {
		await(GroupChatUserDb::instance()->userJids(accountJid, groupChatJid), this, [this, chatController, accountJid, groupChatJid](QVector<QString> &&userJids) {
			// Ensure that the chat is still the open one after fetching the user JIDs.
			if (chatController->accountJid() == accountJid && chatController->chatJid() == groupChatJid) {
				setCurrentUserJids(userJids);
				updateEncryption();
			}
		});
	}
}

void GroupChatController::updateEncryption()
{
	auto chatController = ChatController::instance();
	const auto accountJid = chatController->accountJid();

	QList<QString> jids = { m_currentUserJids.cbegin(), m_currentUserJids.cend() };
	jids.removeOne(accountJid);
	EncryptionController::instance()->initializeChat(accountJid, jids);

	chatController->chatEncryptionWatcher()->setJids({ m_currentUserJids.cbegin(), m_currentUserJids.cend() });
}

void GroupChatController::setCurrentUserJids(const QVector<QString> &currentUserJids)
{
	if (m_currentUserJids != currentUserJids) {
		m_currentUserJids = currentUserJids;
		Q_EMIT currentUserJidsChanged();
	}
}
