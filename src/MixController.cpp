// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MixController.h"

// QXmpp
#include <QXmppClient.h>
#include <QXmppMixInvitation.h>
#include <QXmppMixParticipantItem.h>
#include <QXmppTask.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "Algorithms.h"
#include "ChatController.h"
#include "ClientController.h"
#include "GroupChatController.h"
#include "GroupChatUser.h"
#include "MainController.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "QmlUtils.h"
#include "RosterDb.h"

MixController::MixController(AccountSettings *accountSettings,
                             GroupChatController *groupChatController,
                             MessageController *messageController,
                             QXmppMixManager *mixManager,
                             QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_groupChatController(groupChatController)
    , m_messageController(messageController)
    , m_manager(mixManager)
{
    connect(m_manager, &QXmppMixManager::participantSupportChanged, this, &MixController::handleParticipantSupportChanged);
    connect(m_manager, &QXmppMixManager::servicesChanged, this, &MixController::handleServicesChanged);

    connect(m_manager, &QXmppMixManager::channelInformationUpdated, this, &MixController::handleChannelInformationUpdated);

    connect(m_manager, &QXmppMixManager::jidAllowed, this, &MixController::handleJidAllowed);
    connect(m_manager, &QXmppMixManager::jidDisallowed, this, &MixController::handleJidDisallowed);

    connect(m_manager, &QXmppMixManager::jidBanned, this, &MixController::handleJidBanned);
    connect(m_manager, &QXmppMixManager::jidUnbanned, this, &MixController::handleJidUnbanned);

    connect(m_manager, &QXmppMixManager::participantReceived, this, &MixController::handleParticipantReceived);
    connect(m_manager, &QXmppMixManager::participantLeft, this, &MixController::handleParticipantLeft);

    connect(m_manager, &QXmppMixManager::channelDeleted, this, &MixController::handleChannelDeleted);
}

bool MixController::channelParticipationSupported() const
{
    return m_channelParticipationSupported;
}

QList<GroupChatService> MixController::groupChatServices() const
{
    return transform<QList<GroupChatService>>(m_services, [](const QXmppMixManager::Service &mixService) -> GroupChatService {
        return {
            .jid = mixService.jid,
            .groupChatsSearchable = mixService.channelsSearchable,
            .groupChatCreationSupported = mixService.channelCreationAllowed,
        };
    });
}

void MixController::createPrivateChannel(const QString &serviceJid)
{
    m_manager->createChannel(serviceJid).then(this, [this, serviceJid](QXmppMixManager::CreationResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            QString errorMessage;

            if (error->isStanzaError()) {
                const auto stanzaError = error->value<QXmppStanza::Error>();
                errorMessage = stanzaError->text();
            } else {
                errorMessage = error->description;
            }

            Q_EMIT m_groupChatController->privateGroupChatCreationFailed(serviceJid, errorMessage);
        } else {
            const auto channelJid = std::get<QXmppMixManager::ChannelJid>(result);
            Q_EMIT m_groupChatController->groupChatCreated(channelJid);
        }
    });
}

void MixController::createPublicChannel(const QString &channelJid)
{
    m_manager->createChannel(QXmppUtils::jidToDomain(channelJid), QXmppUtils::jidToUser(channelJid))
        .then(this, [this, channelJid](QXmppMixManager::CreationResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                QString errorMessage;

                if (error->isStanzaError()) {
                    const auto stanzaError = error->value<QXmppStanza::Error>();

                    switch (stanzaError->type()) {
                    case QXmppStanza::Error::Cancel:
                        if (stanzaError->condition() == QXmppStanza::Error::Conflict || stanzaError->condition() == QXmppStanza::Error::NotAllowed)
                            errorMessage = tr("The group already exists");
                        break;
                    default:
                        errorMessage = stanzaError->text();
                    }
                } else {
                    errorMessage = error->description;
                }

                Q_EMIT m_groupChatController->publicGroupChatCreationFailed(channelJid, errorMessage);
            } else {
                Q_EMIT m_groupChatController->groupChatCreated(channelJid);
            }
        });
}

void MixController::requestChannelAccessibility(const QString &channelJid)
{
    m_manager->requestChannelJids(QXmppUtils::jidToDomain(channelJid)).then(this, [this, channelJid](QXmppMixManager::ChannelJidResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(
                tr("Whether %1 is public could not be determined: %2").arg(channelJid, error->description));
        } else if (const auto channelJids = std::get<QList<QXmppMixManager::ChannelJid>>(result); channelJids.contains(channelJid)) {
            m_groupChatController->groupChatMadePublic(channelJid);
        } else {
            m_groupChatController->groupChatMadePrivate(channelJid);
        }
    });
}

void MixController::requestChannelInformation(const QString &channelJid)
{
    m_manager->requestChannelInformation(channelJid).then(this, [this, channelJid](QXmppMixManager::InformationResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(
                tr("Could not retrieve information of group %1: %2").arg(channelJid, error->description));
        } else {
            handleChannelInformationUpdated(channelJid, std::get<QXmppMixInfoItem>(result));
        }
    });
}

void MixController::renameChannel(const QString &channelJid, const QString &newChannelName)
{
    m_manager->requestChannelInformation(channelJid).then(this, [this, channelJid, newChannelName](QXmppMixManager::InformationResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not rename group %1: %2").arg(channelJid, error->description));
        } else {
            auto information = std::get<QXmppMixInfoItem>(result);
            information.setName(newChannelName);

            auto future = m_manager->updateChannelInformation(channelJid, information);
            future.then(this, [channelJid, newChannelName](QXmppClient::EmptyResult &&result) {
                if (const auto error = std::get_if<QXmppError>(&result)) {
                    Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not rename group %1: %2").arg(channelJid, error->description));
                }
            });
        }
    });
}

void MixController::joinChannel(const QString &channelJid, const QString &nickname)
{
    m_manager->joinChannel(channelJid, nickname).then(this, [this, channelJid](QXmppMixManager::JoiningResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            QString errorMessage;

            if (error->isStanzaError()) {
                const auto stanzaError = error->value<QXmppStanza::Error>();

                switch (stanzaError->type()) {
                case QXmppStanza::Error::Cancel:
                    switch (stanzaError->condition()) {
                    case QXmppStanza::Error::ItemNotFound:
                        errorMessage = tr("The group does not exist");
                        break;
                    case QXmppStanza::Error::NotAllowed:
                        errorMessage = tr("You aren't allowed to join the group");
                        break;
                    default:
                        break;
                    }

                    break;
                default:
                    errorMessage = stanzaError->text();
                }
            } else {
                errorMessage = error->description;
            }

            Q_EMIT m_groupChatController->groupChatJoiningFailed(channelJid, errorMessage);
        } else {
            Q_EMIT m_groupChatController->groupChatJoined(channelJid);
        }
    });
}

void MixController::inviteContactToChannel(const QString &channelJid, const QString &contactJid, bool channelPublic)
{
    const auto accountJid = m_accountSettings->jid();

    GroupChatInvitation groupChatInvitation{
        .inviterJid = accountJid,
        .inviteeJid = contactJid,
        .groupChatJid = channelJid,
        .token = {},
    };

    Message message;
    message.accountJid = accountJid;
    message.chatJid = contactJid;
    message.isOwn = true;
    message.id = QXmppUtils::generateStanzaUuid();
    message.originId = message.id;
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.setPreparedBody(QmlUtils::groupChatUriString(channelJid));
    message.deliveryState = DeliveryState::Pending;
    message.receiptRequested = true;
    message.groupChatInvitation = groupChatInvitation;

    auto sendInvitation = [this, message]() {
        m_messageController->sendMessageWithUndecidedEncryption(std::move(message));
    };

    if (channelPublic) {
        sendInvitation();
    } else {
        m_manager->requestChannelNodes(channelJid).then(this, [this, sendInvitation, channelJid, contactJid](QXmppMixManager::ChannelNodeResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Q_EMIT MainController::instance()->passiveNotificationRequested(
                    tr("%1 could not be invited to %2: %3").arg(contactJid, channelJid, error->description));
            } else {
                auto allowJid = [this, sendInvitation, channelJid, contactJid]() {
                    m_manager->allowJid(channelJid, contactJid).then(this, [sendInvitation, channelJid, contactJid](QXmppClient::EmptyResult &&result) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            Q_EMIT MainController::instance()->passiveNotificationRequested(
                                tr("%1 could not be invited to %2: %3").arg(contactJid, channelJid, error->description));
                        } else {
                            // Invitations are only sent to real users.
                            // Invitations are not sent to domains which are only used to restrict the channel's membership to JIDs of that domain.
                            if (!QXmppUtils::jidToUser(contactJid).isEmpty()) {
                                sendInvitation();
                            }
                        }
                    });
                };

                const auto nodes = std::get<QXmppMixConfigItem::Nodes>(result);
                if (nodes.testFlag(QXmppMixConfigItem::Node::AllowedJids)) {
                    allowJid();
                } else {
                    sendInvitation();
                }
            }
        });
    }
}

void MixController::requestChannelUsers(const QString &channelJid)
{
    m_manager->requestChannelNodes(channelJid).then(this, [this, channelJid](QXmppMixManager::ChannelNodeResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Nodes of %1 could not be retrieved: %2").arg(channelJid, error->description));
        } else {
            const auto nodes = std::get<QXmppMixConfigItem::Nodes>(result);

            if (nodes.testFlag(QXmppMixConfigItem::Node::AllowedJids)) {
                m_manager->requestAllowedJids(channelJid).then(this, [this, channelJid](QXmppMixManager::JidResult &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        Q_EMIT MainController::instance()->passiveNotificationRequested(
                            tr("Allowed users of %1 could not be retrieved: %2").arg(channelJid, error->description));
                    } else {
                        const auto jids = std::get<QList<QXmppMixManager::Jid>>(result);
                        for (const auto &jid : jids) {
                            handleJidAllowed(channelJid, jid);
                        }
                    }
                });
            }

            if (nodes.testFlag(QXmppMixConfigItem::Node::BannedJids)) {
                m_manager->requestBannedJids(channelJid).then(this, [this, channelJid](QXmppMixManager::JidResult &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        Q_EMIT MainController::instance()->passiveNotificationRequested(
                            tr("Banned users of %1 could not be retrieved: %2").arg(channelJid, error->description));
                    } else {
                        const auto jids = std::get<QList<QXmppMixManager::Jid>>(result);
                        for (const auto &jid : jids) {
                            handleJidBanned(channelJid, jid);
                        }
                    }
                });
            }
        }
    });

    m_manager->requestParticipants(channelJid).then(this, [this, channelJid](QXmppMixManager::ParticipantResult result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(
                tr("Joined users of %1 could not be retrieved: %2").arg(channelJid, error->description));
        } else {
            const auto participants = std::get<QList<QXmppMixParticipantItem>>(result);
            for (const auto &participant : participants) {
                handleParticipantReceived(channelJid, participant);
            }
        }
    });
}

void MixController::banUser(const QString &channelJid, const QString &userJid)
{
    m_manager->requestChannelConfiguration(channelJid).then(m_manager, [this, channelJid, userJid](QXmppMixManager::ConfigurationResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT MainController::instance()->passiveNotificationRequested(
                tr("%1 could not be banned from %2: %3").arg(userJid, channelJid, error->description));
        } else {
            auto channelConfiguration = std::get<QXmppMixConfigItem>(result);
            auto banJid = [this, channelJid, userJid]() {
                m_manager->banJid(channelJid, userJid);
            };

            if (channelConfiguration.nodes().testFlag(QXmppMixConfigItem::Node::BannedJids)) {
                banJid();
            } else {
                channelConfiguration.setNodes(channelConfiguration.nodes() | QXmppMixConfigItem::Node::BannedJids);
                m_manager->updateChannelConfiguration(channelJid, channelConfiguration).then(m_manager, [this, channelJid, banJid](auto) {
                    m_manager->updateSubscriptions(channelJid, {QXmppMixConfigItem::Node::BannedJids}).then(m_manager, [banJid](auto) {
                        banJid();
                    });
                });
            }
        }
    });
}

void MixController::leaveChannel(const QString &channelJid)
{
    m_manager->leaveChannel(channelJid).then(this, [this, channelJid](QXmppClient::EmptyResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT m_groupChatController->groupChatLeavingFailed(channelJid, error->description);
        } else {
            Q_EMIT m_groupChatController->groupChatLeft(channelJid);
        }
    });
}

void MixController::deleteChannel(const QString &channelJid)
{
    m_manager->deleteChannel(channelJid).then(this, [this, channelJid](QXmppClient::EmptyResult &&result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            QString errorMessage;

            if (error->isStanzaError()) {
                const auto stanzaError = error->value<QXmppStanza::Error>();

                switch (stanzaError->type()) {
                case QXmppStanza::Error::Cancel:
                    if (stanzaError->condition() == QXmppStanza::Error::NotAllowed)
                        errorMessage = tr("You aren't allowed to delete the group");
                    break;
                default:
                    errorMessage = stanzaError->text();
                }
            } else {
                errorMessage = error->description;
            }

            Q_EMIT m_groupChatController->groupChatDeletionFailed(channelJid, errorMessage);
        } else {
            leaveChannel(channelJid);
        }
    });
}

void MixController::handleParticipantSupportChanged()
{
    m_channelParticipationSupported = m_manager->participantSupport() == QXmppMixManager::Support::Supported;
    Q_EMIT m_groupChatController->groupChatParticipationSupportedChanged();
}

void MixController::handleServicesChanged()
{
    m_services = m_manager->services();
    Q_EMIT m_groupChatController->groupChatServicesChanged();
}

void MixController::handleChannelInformationUpdated(const QString &channelJid, const QXmppMixInfoItem &information)
{
    RosterDb::instance()->updateItem(m_accountSettings->jid(), channelJid, [information](RosterItem &item) {
        item.groupChatName = information.name();
        item.groupChatDescription = information.description();
    });
}

void MixController::handleChannelAccessibilityReceived(const QString &channelJid, const bool isPublic)
{
    if (isPublic)
        Q_EMIT m_groupChatController->groupChatMadePublic(channelJid);
    else
        Q_EMIT m_groupChatController->groupChatMadePrivate(channelJid);
}

void MixController::handleJidAllowed(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Allowed;

    Q_EMIT m_groupChatController->userAllowedOrBanned(user);
}

void MixController::handleJidDisallowed(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Allowed;

    Q_EMIT m_groupChatController->userDisallowedOrUnbanned(user);
}

void MixController::handleJidBanned(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Banned;

    Q_EMIT m_groupChatController->userAllowedOrBanned(user);
}

void MixController::handleJidUnbanned(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Banned;

    Q_EMIT m_groupChatController->userDisallowedOrUnbanned(user);
}

void MixController::handleParticipantReceived(const QString &channelJid, const QXmppMixParticipantItem &participantItem)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.id = participantItem.id();
    user.jid = participantItem.jid();
    user.name = participantItem.nick();

    Q_EMIT m_groupChatController->participantReceived(user);
}

void MixController::handleParticipantLeft(const QString &channelJid, const QString &participantId)
{
    GroupChatUser user;
    user.accountJid = m_accountSettings->jid();
    user.chatJid = channelJid;
    user.id = participantId;

    Q_EMIT m_groupChatController->participantLeft(user);
}

void MixController::handleChannelDeleted(const QString &channelJid)
{
    Q_EMIT m_groupChatController->groupChatDeleted(channelJid);
}

#include "moc_MixController.cpp"
