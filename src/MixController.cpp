// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "MixController.h"

// Qt
#include <QStringBuilder>
// QXmpp
#include "QXmppMixParticipantItem.h"
#include <QXmppClient.h>
#include <QXmppMixInvitation.h>
#include <QXmppMixManager.h>
#include <QXmppStanza.h>
#include <QXmppTask.h>
#include <QXmppUri.h>
#include <QXmppUtils.h>
// Kaidan
#include "Algorithms.h"
#include "ChatController.h"
#include "ClientWorker.h"
#include "GroupChatController.h"
#include "GroupChatUser.h"
#include "Kaidan.h"
#include "MessageController.h"
#include "MessageDb.h"
#include "RosterDb.h"

MixController::MixController(QObject *parent)
    : QObject(parent)
{
    runOnThread(Kaidan::instance()->client(), [this]() {
        auto manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>();

        connect(manager, &QXmppMixManager::participantSupportChanged, this, &MixController::handleParticipantSupportChanged);
        connect(manager, &QXmppMixManager::servicesChanged, this, &MixController::handleServicesChanged);

        connect(manager, &QXmppMixManager::channelInformationUpdated, this, &MixController::handleChannelInformationUpdated);

        connect(manager, &QXmppMixManager::jidAllowed, this, &MixController::handleJidAllowed);
        connect(manager, &QXmppMixManager::jidDisallowed, this, &MixController::handleJidDisallowed);

        connect(manager, &QXmppMixManager::jidBanned, this, &MixController::handleJidBanned);
        connect(manager, &QXmppMixManager::jidUnbanned, this, &MixController::handleJidUnbanned);

        connect(manager, &QXmppMixManager::participantReceived, this, &MixController::handleParticipantReceived);
        connect(manager, &QXmppMixManager::participantLeft, this, &MixController::handleParticipantLeft);

        connect(manager, &QXmppMixManager::channelDeleted, this, &MixController::handleChannelDeleted);
    });
}

bool MixController::channelParticipationSupported() const
{
    return m_channelParticipationSupported;
}

QList<GroupChatService> MixController::groupChatServices() const
{
    return transform<QList<GroupChatService>>(m_services, [](const QXmppMixManager::Service &mixService) -> GroupChatService {
        return {
            .accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare(),
            .jid = mixService.jid,
            .groupChatsSearchable = mixService.channelsSearchable,
            .groupChatCreationSupported = mixService.channelCreationAllowed,
        };
    });
}

void MixController::createPrivateChannel(const QString &serviceJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, serviceJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->createChannel(serviceJid), this};
        },
        this,
        [serviceJid](QXmppMixManager::CreationResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                QString errorMessage;

                if (error->isStanzaError()) {
                    const auto stanzaError = error->value<QXmppStanza::Error>();
                    errorMessage = stanzaError->text();
                } else {
                    errorMessage = error->description;
                }

                Q_EMIT GroupChatController::instance() -> privateGroupChatCreationFailed(serviceJid, errorMessage);
            } else {
                const auto channelJid = std::get<QXmppMixManager::ChannelJid>(result);
                Q_EMIT GroupChatController::instance() -> groupChatCreated(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
            }
        });
}

void MixController::createPublicChannel(const QString &channelJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->createChannel(QXmppUtils::jidToDomain(channelJid),
                                                                                                                         QXmppUtils::jidToUser(channelJid)),
                             this};
        },
        this,
        [channelJid](QXmppMixManager::CreationResult &&result) {
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

                Q_EMIT GroupChatController::instance() -> publicGroupChatCreationFailed(channelJid, errorMessage);
            } else {
                Q_EMIT GroupChatController::instance() -> groupChatCreated(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
            }
        });
}

void MixController::requestChannelAccessibility(const QString &channelJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            const auto serviceJid = QXmppUtils::jidToDomain(channelJid);
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestChannelJids(serviceJid), this};
        },
        this,
        [channelJid](QXmppMixManager::ChannelJidResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Kaidan::instance()->passiveNotificationRequested(tr("Whether %1 is public could not be determined: %2").arg(channelJid, error->description));
            } else if (const auto channelJids = std::get<QVector<QXmppMixManager::ChannelJid>>(result); channelJids.contains(channelJid)) {
                GroupChatController::instance()->groupChatMadePublic(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
            } else {
                GroupChatController::instance()->groupChatMadePrivate(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
            }
        });
}

void MixController::requestChannelInformation(const QString &channelJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestChannelInformation(channelJid), this};
        },
        this,
        [this, channelJid](QXmppMixManager::InformationResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Q_EMIT Kaidan::instance() -> passiveNotificationRequested(
                                              tr("Could not retrieve information of group %1: %2").arg(channelJid, error->description));
            } else {
                handleChannelInformationUpdated(channelJid, std::get<QXmppMixInfoItem>(result));
            }
        });
}

void MixController::renameChannel(const QString &channelJid, const QString &newChannelName)
{
    runOnThread(Kaidan::instance()->client(), [this, channelJid, newChannelName]() {
        auto manager = Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>();
        manager->requestChannelInformation(channelJid).then(this, [this, channelJid, newChannelName, manager](QXmppMixManager::InformationResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("Could not rename group %1: %2").arg(channelJid, error->description));
            } else {
                auto information = std::get<QXmppMixInfoItem>(result);
                information.setName(newChannelName);

                auto future = manager->updateChannelInformation(channelJid, information);
                future.then(this, [channelJid, newChannelName](QXmppClient::EmptyResult &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        Q_EMIT Kaidan::instance() -> passiveNotificationRequested(tr("Could not rename group %1: %2").arg(channelJid, error->description));
                    }
                });
            }
        });
    });
}

void MixController::joinChannel(const QString &channelJid, const QString &nickname)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid, nickname]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->joinChannel(channelJid, nickname), this};
        },
        this,
        [channelJid](QXmppMixManager::JoiningResult &&result) {
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

                Q_EMIT GroupChatController::instance() -> groupChatJoiningFailed(channelJid, errorMessage);
            } else {
                Q_EMIT GroupChatController::instance() -> groupChatJoined(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
            }
        });
}

void MixController::inviteContactToChannel(const QString &channelJid, const QString &contactJid, bool channelPublic)
{
    const auto ownJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();

    GroupChatInvitation groupChatInvitation{
        .inviterJid = ownJid,
        .inviteeJid = contactJid,
        .groupChatJid = channelJid,
        .token = {},
    };

    QXmppUri groupChatUri;
    groupChatUri.setJid(channelJid);
    groupChatUri.setQuery(QXmpp::Uri::Join());

    Message message;
    message.accountJid = ownJid;
    message.chatJid = contactJid;
    message.isOwn = true;
    message.id = QXmppUtils::generateStanzaUuid();
    message.originId = message.id;
    message.timestamp = QDateTime::currentDateTimeUtc();
    message.body = groupChatUri.toString();
    message.encryption = ChatController::instance()->activeEncryption();
    message.deliveryState = DeliveryState::Pending;
    message.receiptRequested = true;
    message.groupChatInvitation = groupChatInvitation;

    MessageDb::instance()->addMessage(message, MessageOrigin::UserInput);

    auto sendInvitation = [message]() {
        MessageController::instance()->sendPendingMessage(message);
    };

    if (channelPublic) {
        sendInvitation();
    } else {
        callRemoteTask(
            Kaidan::instance()->client(),
            [this, channelJid]() {
                return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestChannelNodes(channelJid), this};
            },
            this,
            [this, sendInvitation, message, channelJid, contactJid](QXmppMixManager::ChannelNodeResult &&result) {
                if (const auto error = std::get_if<QXmppError>(&result)) {
                    Kaidan::instance()->passiveNotificationRequested(tr("%1 could not be invited to %2: %3").arg(contactJid, channelJid, error->description));
                } else {
                    auto allowJid = [this, sendInvitation, message, channelJid, contactJid]() {
                        callRemoteTask(
                            Kaidan::instance()->client(),
                            [this, channelJid, contactJid]() {
                                return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->allowJid(channelJid, contactJid),
                                                 this};
                            },
                            this,
                            [sendInvitation, message, channelJid, contactJid](QXmppClient::EmptyResult &&result) {
                                if (const auto error = std::get_if<QXmppError>(&result)) {
                                    Kaidan::instance()->passiveNotificationRequested(
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
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestChannelNodes(channelJid), this};
        },
        this,
        [this, channelJid](QXmppMixManager::ChannelNodeResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Kaidan::instance()->passiveNotificationRequested(tr("Nodes of %1 could not be retrieved: %2").arg(channelJid, error->description));
            } else {
                const auto nodes = std::get<QXmppMixConfigItem::Nodes>(result);

                if (nodes.testFlag(QXmppMixConfigItem::Node::AllowedJids)) {
                    callRemoteTask(
                        Kaidan::instance()->client(),
                        [this, channelJid]() {
                            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestAllowedJids(channelJid),
                                             this};
                        },
                        this,
                        [this, channelJid](QXmppMixManager::JidResult &&result) {
                            if (const auto error = std::get_if<QXmppError>(&result)) {
                                Kaidan::instance()->passiveNotificationRequested(
                                    tr("Allowed users of %1 could not be retrieved: %2").arg(channelJid, error->description));
                            } else {
                                const auto jids = std::get<QVector<QXmppMixManager::Jid>>(result);
                                for (const auto &jid : jids) {
                                    handleJidAllowed(channelJid, jid);
                                }
                            }
                        });
                }

                if (nodes.testFlag(QXmppMixConfigItem::Node::BannedJids)) {
                    callRemoteTask(
                        Kaidan::instance()->client(),
                        [this, channelJid]() {
                            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestBannedJids(channelJid), this};
                        },
                        this,
                        [this, channelJid](QXmppMixManager::JidResult &&result) {
                            if (const auto error = std::get_if<QXmppError>(&result)) {
                                Kaidan::instance()->passiveNotificationRequested(
                                    tr("Banned users of %1 could not be retrieved: %2").arg(channelJid, error->description));
                            } else {
                                const auto jids = std::get<QVector<QXmppMixManager::Jid>>(result);
                                for (const auto &jid : jids) {
                                    handleJidBanned(channelJid, jid);
                                }
                            }
                        });
                }
            }
        });

    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->requestParticipants(channelJid), this};
        },
        this,
        [this, channelJid](QXmppMixManager::ParticipantResult result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Kaidan::instance()->passiveNotificationRequested(tr("Joined users of %1 could not be retrieved: %2").arg(channelJid, error->description));
            } else {
                const auto participants = std::get<QVector<QXmppMixParticipantItem>>(result);
                for (const auto &participant : participants) {
                    handleParticipantReceived(channelJid, participant);
                }
            }
        });
}

void MixController::banUser(const QString &channelJid, const QString &userJid)
{
    runOnThread(Kaidan::instance()->client(), [channelJid, userJid]() {
        auto client = Kaidan::instance()->client();
        auto manager = client->xmppClient()->findExtension<QXmppMixManager>();

        manager->requestChannelConfiguration(channelJid).then(client, [channelJid, userJid, client, manager](QXmppMixManager::ConfigurationResult &&result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Kaidan::instance()->passiveNotificationRequested(tr("%1 could not be banned from %2: %3").arg(userJid, channelJid, error->description));
            } else {
                auto channelConfiguration = std::get<QXmppMixConfigItem>(result);
                auto banJid = [channelJid, userJid, manager]() {
                    manager->banJid(channelJid, userJid);
                };

                if (channelConfiguration.nodes().testFlag(QXmppMixConfigItem::Node::BannedJids)) {
                    banJid();
                } else {
                    channelConfiguration.setNodes(channelConfiguration.nodes() | QXmppMixConfigItem::Node::BannedJids);
                    manager->updateChannelConfiguration(channelJid, channelConfiguration).then(client, [client, channelJid, banJid](auto) {
                        Kaidan::instance()
                            ->client()
                            ->xmppClient()
                            ->findExtension<QXmppMixManager>()
                            ->updateSubscriptions(channelJid, {QXmppMixConfigItem::Node::BannedJids})
                            .then(client, [banJid](auto) {
                                banJid();
                            });
                    });
                }
            }
        });
    });
}

void MixController::leaveChannel(const QString &channelJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->leaveChannel(channelJid), this};
        },
        this,
        [channelJid](QXmppClient::EmptyResult &&result) {
            const auto ownJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();

            if (const auto error = std::get_if<QXmppError>(&result)) {
                Q_EMIT GroupChatController::instance() -> groupChatLeavingFailed(ownJid, channelJid, error->description);
            } else {
                Q_EMIT GroupChatController::instance() -> groupChatLeft(ownJid, channelJid);
            }
        });
}

void MixController::deleteChannel(const QString &channelJid)
{
    callRemoteTask(
        Kaidan::instance()->client(),
        [this, channelJid]() {
            return std::pair{Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->deleteChannel(channelJid), this};
        },
        this,
        [this, channelJid](QXmppClient::EmptyResult &&result) {
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

                Q_EMIT GroupChatController::instance() -> groupChatDeletionFailed(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(),
                                                                                  channelJid,
                                                                                  errorMessage);
            } else {
                leaveChannel(channelJid);
            }
        });
}

void MixController::handleParticipantSupportChanged()
{
    runOnThread(
        Kaidan::instance()->client(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->participantSupport();
        },
        this,
        [this](QXmppMixManager::Support participantSupport) {
            m_channelParticipationSupported = (participantSupport == QXmppMixManager::Support::Supported);
            Q_EMIT GroupChatController::instance() -> groupChatParticipationSupportedChanged();
        });
}

void MixController::handleServicesChanged()
{
    runOnThread(
        Kaidan::instance()->client(),
        []() {
            return Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->services();
        },
        this,
        [this](const QList<QXmppMixManager::Service> &&services) {
            m_services = services;
            Q_EMIT GroupChatController::instance() -> groupChatServicesChanged();
        });
}

void MixController::handleChannelInformationUpdated(const QString &channelJid, const QXmppMixInfoItem &information)
{
    RosterDb::instance()->updateItem(channelJid, [information](RosterItem &item) {
        item.groupChatName = information.name();
        item.groupChatDescription = information.description();
    });
}

void MixController::handleChannelAccessibilityReceived(const QString &channelJid, const bool isPublic)
{
    if (isPublic)
        Q_EMIT GroupChatController::instance() -> groupChatMadePublic(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
    else
        Q_EMIT GroupChatController::instance() -> groupChatMadePrivate(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
}

void MixController::handleChannelMadePrivate(const QString &channelJid)
{
    runOnThread(Kaidan::instance()->client(), [channelJid]() {
        Kaidan::instance()->client()->xmppClient()->findExtension<QXmppMixManager>()->updateSubscriptions(channelJid);
    });

    Q_EMIT GroupChatController::instance() -> groupChatMadePrivate(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
}

void MixController::handleJidAllowed(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Allowed;

    Q_EMIT GroupChatController::instance() -> userAllowedOrBanned(user);
}

void MixController::handleJidDisallowed(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Allowed;

    Q_EMIT GroupChatController::instance() -> userDisallowedOrUnbanned(user);
}

void MixController::handleJidBanned(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Banned;

    Q_EMIT GroupChatController::instance() -> userAllowedOrBanned(user);
}

void MixController::handleJidUnbanned(const QString &channelJid, const QString &jid)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.jid = jid;
    user.status = GroupChatUser::Status::Banned;

    Q_EMIT GroupChatController::instance() -> userDisallowedOrUnbanned(user);
}

void MixController::handleParticipantReceived(const QString &channelJid, const QXmppMixParticipantItem &participantItem)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.id = participantItem.id();
    user.jid = participantItem.jid();
    user.name = participantItem.nick();

    Q_EMIT GroupChatController::instance() -> participantReceived(user);
}

void MixController::handleParticipantLeft(const QString &channelJid, const QString &participantId)
{
    GroupChatUser user;
    user.accountJid = Kaidan::instance()->client()->xmppClient()->configuration().jidBare();
    user.chatJid = channelJid;
    user.id = participantId;

    Q_EMIT GroupChatController::instance() -> participantLeft(user);
}

void MixController::handleChannelDeleted(const QString &channelJid)
{
    Q_EMIT GroupChatController::instance() -> groupChatDeleted(Kaidan::instance()->client()->xmppClient()->configuration().jidBare(), channelJid);
}
