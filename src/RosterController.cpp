// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterController.h"

// QXmpp
#include <QXmppRosterManager.h>
#include <QXmppTask.h>
// Kaidan
#include "AccountController.h"
#include "AvatarFileStorage.h"
#include "ChatHintModel.h"
#include "EncryptionController.h"
#include "GroupChatController.h"
#include "KaidanCoreLog.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterModel.h"
#include "VCardController.h"

RosterController::RosterController(QObject *parent)
    : QObject(parent)
    , m_clientWorker(Kaidan::instance()->client())
    , m_client(m_clientWorker->xmppClient())
    , m_manager(m_clientWorker->rosterManager())
{
    connect(m_manager, &QXmppRosterManager::rosterReceived, this, &RosterController::populateRoster);

    connect(m_manager, &QXmppRosterManager::itemAdded, this, [this](const QString &jid) {
        runOnThread(
            m_client,
            [this, jid]() {
                return std::tuple{m_client->configuration().jidBare(), m_manager->getRosterEntry(jid)};
            },
            this,
            [this, jid](std::tuple<QString, QXmppRosterIq::Item> &&result) {
                auto [accountJid, addedItem] = result;
                RosterItem rosterItem{accountJid, addedItem};
                rosterItem.encryption = AccountController::instance()->account().encryption;
                rosterItem.lastMessageDateTime = QDateTime::currentDateTimeUtc();
                RosterDb::instance()->addItem(rosterItem);

                // Requesting the group chat's information is done here and not within the joining method of
                // the group chat controller to cover both cases:
                //   1. This client joined the group chat (could be done during joining).
                //   2. Another own client joined the group chat (must be covered here).
                if (rosterItem.isGroupChat()) {
                    GroupChatController::instance()->requestGroupChatAccessibility(rosterItem.accountJid, rosterItem.jid);
                    GroupChatController::instance()->requestChannelInformation(rosterItem.accountJid, rosterItem.jid);
                }

                if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
                    Kaidan::instance()->vCardController()->requestVCard(jid);
                }

                if (m_pendingSubscriptionRequests.contains(jid)) {
                    const auto subscriptionRequest = m_pendingSubscriptionRequests.take(jid);
                    applyOldContactData(subscriptionRequest.oldJid(), jid);
                    addUnrespondedSubscriptionRequest(jid, subscriptionRequest);
                }
            });
    });

    connect(m_manager, &QXmppRosterManager::itemChanged, this, [this](const QString &jid) {
        runOnThread(
            m_manager,
            [this, jid]() {
                return m_manager->getRosterEntry(jid);
            },
            this,
            [this, jid](QXmppRosterIq::Item &&changedItem) {
                RosterDb::instance()->updateItem(jid, [jid, changedItem](RosterItem &item) {
                    item.name = changedItem.name();
                    item.subscription = changedItem.subscriptionType();

                    const auto groups = changedItem.groups();
                    item.groups = QList(groups.cbegin(), groups.cend());
                });

                if (m_isItemBeingChanged) {
                    runOnThread(m_clientWorker, [this]() {
                        m_clientWorker->finishTask();
                    });

                    m_isItemBeingChanged = false;
                }
            });
    });

    connect(m_manager, &QXmppRosterManager::itemRemoved, this, [this](const QString &jid) {
        runOnThread(
            m_client,
            [this, jid]() {
                return m_client->configuration().jidBare();
            },
            this,
            [jid](QString &&accountJid) {
                MessageDb::instance()->removeAllMessagesFromChat(accountJid, jid);
                RosterDb::instance()->removeItem(accountJid, jid);
                EncryptionController::instance()->removeContactDevices(jid);
            });
    });

    connect(m_manager, &QXmppRosterManager::subscriptionRequestReceived, this, &RosterController::handleSubscriptionRequest);
}

void RosterController::populateRoster()
{
    qCDebug(KAIDAN_CORE_LOG) << "Populating roster";

    runOnThread(
        m_manager,
        [this]() {
            QHash<QString, RosterItem> rosterItems;
            const auto accountJid = m_client->configuration().jidBare();
            const auto jids = m_manager->getRosterBareJids();

            for (const auto &jid : jids) {
                rosterItems.insert(jid, {accountJid, m_manager->getRosterEntry(jid)});
            }

            return rosterItems;
        },
        this,
        [this](QHash<QString, RosterItem> &&rosterItems) {
            // // create a new list of contacts
            // QHash<QString, RosterItem> items;

            for (auto itr = rosterItems.begin(); itr != rosterItems.end(); ++itr) {
                const auto jid = itr.key();

                itr->encryption = AccountController::instance()->account().encryption;

                if (Kaidan::instance()->avatarStorage()->getHashOfJid(jid).isEmpty()
                    && Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
                    Kaidan::instance()->vCardController()->requestVCard(jid);
                }

                // Process subscription requests from roster items that were received before the roster was
                // received.
                if (m_unprocessedSubscriptionRequests.contains(jid)) {
                    addUnrespondedSubscriptionRequest(jid, m_unprocessedSubscriptionRequests.take(jid));
                }
            }

            // replace current contacts with new ones from server
            RosterDb::instance()->replaceItems(rosterItems);

            // Process subscription requests from strangers that were received before the roster was
            // received.
            for (auto itr = m_unprocessedSubscriptionRequests.begin(); itr != m_unprocessedSubscriptionRequests.end();) {
                processSubscriptionRequestFromStranger(itr.key(), itr.value());
                itr = m_unprocessedSubscriptionRequests.erase(itr);
            }
        });
}

void RosterController::handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request)
{
    runOnThread(
        m_manager,
        [this]() {
            return m_manager->isRosterReceived();
        },
        this,
        [this, subscriberJid, request](bool rosterReceived) {
            if (rosterReceived) {
                runOnThread(
                    m_manager,
                    [this, subscriberJid]() {
                        return m_manager->getRosterBareJids().contains(subscriberJid);
                    },
                    this,
                    [this, subscriberJid, request](bool isSubscriberInRoster) {
                        if (isSubscriberInRoster) {
                            addUnrespondedSubscriptionRequest(subscriberJid, request);
                        } else {
                            processSubscriptionRequestFromStranger(subscriberJid, request);
                        }
                    });
            } else {
                m_unprocessedSubscriptionRequests.insert(subscriberJid, request);
            }
        });
}

void RosterController::processSubscriptionRequestFromStranger(const QString &subscriberJid, const QXmppPresence &request)
{
    m_pendingSubscriptionRequests.insert(subscriberJid, request);
    addContact(subscriberJid);
}

void RosterController::addUnrespondedSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request)
{
    m_unrespondedSubscriptionRequests.insert(subscriberJid, request);

    runOnThread(m_client, [this, request]() {
        Q_EMIT presenceSubscriptionRequestReceived(m_client->configuration().jidBare(), request);
    });
}

void RosterController::addContact(const QString &jid, const QString &name, const QString &message, bool automaticInitialAddition)
{
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_client, [this, jid, name, message, automaticInitialAddition, addingOwnJidToRosterAllowed = m_addingOwnJidToRosterAllowed]() {
            // Do not try to add the own JID to the roster mutiple times if the server does not support
            // it.
            if (const auto ownJidBeingAdded = jid == m_client->configuration().jidBare(); ownJidBeingAdded && !addingOwnJidToRosterAllowed) {
                if (!automaticInitialAddition) {
                    Q_EMIT contactAdditionFailed(jid, jid);
                }
            } else {
                m_manager->addRosterItem(jid, name).then(
                    this,
                    [this, jid, message, automaticInitialAddition, ownJidBeingAdded](QXmppRosterManager::Result &&result) {
                        if (const auto error = std::get_if<QXmppError>(&result)) {
                            if (const auto stanzaError = error->takeValue<QXmppStanza::Error>(); ownJidBeingAdded && stanzaError
                                && stanzaError->type() == QXmppStanza::Error::Cancel && stanzaError->condition() == QXmppStanza::Error::NotAllowed) {
                                m_addingOwnJidToRosterAllowed = false;

                                if (automaticInitialAddition) {
                                    return;
                                }

                                Q_EMIT contactAdditionFailed(jid, jid);

                                return;
                            }

                            Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("%1 could not be added: %2").arg(jid, error->description));
                        } else {
                            if (!ownJidBeingAdded) {
                                m_manager->subscribeTo(jid, message);
                            }
                        }
                    });
            }
        });
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not add contact as a result of not being connected."));
        qCWarning(KAIDAN_CORE_LOG) << "Could not add contact as a result of not being connected.";
    }
}

void RosterController::removeContact(const QString &jid)
{
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this, jid]() {
            m_manager->removeItem(jid);
        });
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not remove contact as a result of not being connected."));
        qCWarning(KAIDAN_CORE_LOG) << "Could not remove contact as a result of not being connected.";
    }
}

void RosterController::renameContact(const QString &jid, const QString &newContactName)
{
    if (Kaidan::instance()->connectionState() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this, jid, newContactName]() {
            m_manager->renameItem(jid, newContactName);
        });
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not rename contact as a result of not being connected."));
        qCWarning(KAIDAN_CORE_LOG) << "Could not rename contact as a result of not being connected.";
    }
}

void RosterController::subscribeToPresence(const QString &contactJid)
{
    callRemoteTask(
        m_manager,
        [this, contactJid]() {
            return std::pair{m_manager->subscribeTo(contactJid), this};
        },
        this,
        [contactJid](QXmpp::SendResult result) {
            if (const auto error = std::get_if<QXmppError>(&result)) {
                Q_EMIT Kaidan::instance()->passiveNotificationRequested(
                    tr("Requesting to see the personal data of %1 failed because of a connection problem: %2").arg(contactJid, error->description));
            }
        });
}

QFuture<bool> RosterController::acceptSubscriptionToPresence(const QString &contactJid)
{
    return runAsync(m_manager,
                    [this, contactJid]() {
                        return m_manager->acceptSubscription(contactJid);
                    })
        .then(this, [this, contactJid](bool succeeded) {
            if (succeeded) {
                m_unrespondedSubscriptionRequests.remove(contactJid);
                return true;
            } else {
                Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Allowing %1 to see your personal data failed").arg(contactJid));
                return false;
            }
        });
}

QFuture<bool> RosterController::refuseSubscriptionToPresence(const QString &contactJid)
{
    return runAsync(m_manager,
                    [this, contactJid]() {
                        return m_manager->refuseSubscription(contactJid);
                    })
        .then(this, [this, contactJid](bool succeeded) {
            if (succeeded) {
                m_unrespondedSubscriptionRequests.remove(contactJid);
                return true;
            } else {
                Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Stopping %1 to see your personal data failed").arg(contactJid));
                return false;
            }
        });
}

QMap<QString, QXmppPresence> RosterController::unrespondedPresenceSubscriptionRequests()
{
    return m_unrespondedSubscriptionRequests;
}

void RosterController::updateGroups(const QString &jid, const QString &name, const QList<QString> &groups)
{
    m_isItemBeingChanged = true;

    runOnThread(m_clientWorker, [this, jid, name, groups]() {
        m_clientWorker->startTask([this, jid, name, groups] {
            // TODO: Add updating only groups to QXmppRosterManager without the need to pass the unmodified name
            m_manager->addItem(jid, name, QSet(groups.cbegin(), groups.cend()));
        });
    });
}

void RosterController::applyOldContactData(const QString &oldContactJid, const QString &newContactJid)
{
    if (oldContactJid.isEmpty()) {
        return;
    }

    if (const auto oldItem = RosterModel::instance()->findItem(oldContactJid)) {
        RosterDb::instance()->updateItem(newContactJid, [oldItem = *oldItem](RosterItem &newItem) {
            newItem.pinningPosition = oldItem.pinningPosition;
            newItem.chatStateSendingEnabled = oldItem.chatStateSendingEnabled;
            newItem.readMarkerSendingEnabled = oldItem.readMarkerSendingEnabled;
            newItem.encryption = oldItem.encryption;
            newItem.notificationRule = oldItem.notificationRule;
            newItem.automaticMediaDownloadsRule = oldItem.automaticMediaDownloadsRule;
        });

        updateGroups(newContactJid, oldItem->name, oldItem->groups);
    }
}

#include "moc_RosterController.cpp"
