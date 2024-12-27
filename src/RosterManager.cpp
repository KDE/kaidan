// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "RosterManager.h"

// QXmpp
#include <QXmppRosterManager.h>
#include <QXmppTask.h>
// Kaidan
#include "AvatarFileStorage.h"
#include "ChatHintModel.h"
#include "EncryptionController.h"
#include "GroupChatController.h"
#include "Kaidan.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterModel.h"
#include "Settings.h"
#include "VCardManager.h"

RosterManager::RosterManager(ClientWorker *clientWorker, QXmppClient *client, QObject *parent)
    : QObject(parent)
    , m_clientWorker(clientWorker)
    , m_client(client)
    , m_avatarStorage(clientWorker->caches()->avatarStorage)
    , m_vCardManager(clientWorker->vCardManager())
    , m_manager(client->findExtension<QXmppRosterManager>())
{
    connect(m_manager, &QXmppRosterManager::rosterReceived, this, &RosterManager::populateRoster);

    connect(m_manager, &QXmppRosterManager::itemAdded, this, [this](const QString &jid) {
        RosterItem rosterItem{m_client->configuration().jidBare(), m_manager->getRosterEntry(jid)};
        rosterItem.encryption = Kaidan::instance()->settings()->encryption();
        rosterItem.automaticMediaDownloadsRule = RosterItem::AutomaticMediaDownloadsRule::Default;
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

        if (m_client->state() == QXmppClient::ConnectedState) {
            m_vCardManager->requestVCard(jid);
        }

        if (m_pendingSubscriptionRequests.contains(jid)) {
            const auto subscriptionRequest = m_pendingSubscriptionRequests.take(jid);
            applyOldContactData(subscriptionRequest.oldJid(), jid);
            addUnrespondedSubscriptionRequest(jid, subscriptionRequest);
        }
    });

    connect(m_manager, &QXmppRosterManager::itemChanged, this, [this](const QString &jid) {
        RosterDb::instance()->updateItem(jid, [jid, updatedItem = m_manager->getRosterEntry(jid)](RosterItem &item) {
            item.name = updatedItem.name();
            item.subscription = updatedItem.subscriptionType();

            const auto groups = updatedItem.groups();
            item.groups = QList(groups.cbegin(), groups.cend());
        });

        if (m_isItemBeingChanged) {
            m_clientWorker->finishTask();
            m_isItemBeingChanged = false;
        }
    });

    connect(m_manager, &QXmppRosterManager::itemRemoved, this, [this](const QString &jid) {
        const auto accountJid = m_client->configuration().jidBare();
        MessageDb::instance()->removeAllMessagesFromChat(accountJid, jid);
        RosterDb::instance()->removeItem(accountJid, jid);

        runOnThread(EncryptionController::instance(), [jid]() {
            EncryptionController::instance()->removeContactDevices(jid);
        });
    });

    connect(m_manager, &QXmppRosterManager::subscriptionRequestReceived, this, &RosterManager::handleSubscriptionRequest);

    // user actions
    connect(this, &RosterManager::addContactRequested, this, &RosterManager::addContact);
    connect(this, &RosterManager::removeContactRequested, this, &RosterManager::removeContact);
    connect(this, &RosterManager::renameContactRequested, this, &RosterManager::renameContact);

    connect(this, &RosterManager::subscribeToPresenceRequested, this, &RosterManager::subscribeToPresence);
    connect(this, &RosterManager::acceptSubscriptionToPresenceRequested, this, &RosterManager::acceptSubscriptionToPresence);
    connect(this, &RosterManager::refuseSubscriptionToPresenceRequested, this, &RosterManager::refuseSubscriptionToPresence);

    connect(this, &RosterManager::updateGroupsRequested, this, &RosterManager::updateGroups);
}

void RosterManager::populateRoster()
{
    qDebug() << "[client] [RosterManager] Populating roster";

    // create a new list of contacts
    QHash<QString, RosterItem> items;
    const QStringList bareJids = m_manager->getRosterBareJids();

    for (const auto &jid : bareJids) {
        RosterItem rosterItem{m_client->configuration().jidBare(), m_manager->getRosterEntry(jid)};
        rosterItem.encryption = Kaidan::instance()->settings()->encryption();
        rosterItem.automaticMediaDownloadsRule = RosterItem::AutomaticMediaDownloadsRule::Default;
        items.insert(jid, rosterItem);

        if (m_avatarStorage->getHashOfJid(jid).isEmpty() && m_client->state() == QXmppClient::ConnectedState) {
            m_vCardManager->requestVCard(jid);
        }

        // Process subscription requests from roster items that were received before the roster was
        // received.
        if (m_unprocessedSubscriptionRequests.contains(jid)) {
            addUnrespondedSubscriptionRequest(jid, m_unprocessedSubscriptionRequests.take(jid));
        }
    }

    // replace current contacts with new ones from server
    RosterDb::instance()->replaceItems(items);

    // Process subscription requests from strangers that were received before the roster was
    // received.
    for (auto itr = m_unprocessedSubscriptionRequests.begin(); itr != m_unprocessedSubscriptionRequests.end();) {
        processSubscriptionRequestFromStranger(itr.key(), itr.value());
        itr = m_unprocessedSubscriptionRequests.erase(itr);
    }
}

void RosterManager::handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request)
{
    if (m_manager->isRosterReceived()) {
        if (m_manager->getRosterBareJids().contains(subscriberJid)) {
            addUnrespondedSubscriptionRequest(subscriberJid, request);
        } else {
            processSubscriptionRequestFromStranger(subscriberJid, request);
        }
    } else {
        m_unprocessedSubscriptionRequests.insert(subscriberJid, request);
    }
}

void RosterManager::processSubscriptionRequestFromStranger(const QString &subscriberJid, const QXmppPresence &request)
{
    m_pendingSubscriptionRequests.insert(subscriberJid, request);
    addContact(subscriberJid);
}

void RosterManager::addUnrespondedSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request)
{
    m_unrespondedSubscriptionRequests.insert(subscriberJid, request);
    Q_EMIT presenceSubscriptionRequestReceived(m_client->configuration().jidBare(), request);
}

void RosterManager::addContact(const QString &jid, const QString &name, const QString &message, bool automaticInitialAddition)
{
    if (m_client->state() == QXmppClient::ConnectedState) {
        // Do not try to add the own JID to the roster mutiple times if the server does not support
        // it.
        if (const auto ownJidBeingAdded = jid == m_client->configuration().jidBare(); ownJidBeingAdded && !m_addingOwnJidToRosterAllowed) {
            if (!automaticInitialAddition) {
                Q_EMIT RosterModel::instance()->itemAdditionFailed(jid, jid);
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

                            Q_EMIT RosterModel::instance()->itemAdditionFailed(jid, jid);

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
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not add contact, as a result of not being connected."));
        qWarning() << "[client] [RosterManager] Could not add contact, as a result of "
                      "not being connected.";
    }
}

void RosterManager::removeContact(const QString &jid)
{
    if (m_client->state() == QXmppClient::ConnectedState) {
        m_manager->removeItem(jid);
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not remove contact, as a result of not being connected."));
        qWarning() << "[client] [RosterManager] Could not remove contact, as a result of "
                      "not being connected.";
    }
}

void RosterManager::renameContact(const QString &jid, const QString &newContactName)
{
    if (m_client->state() == QXmppClient::ConnectedState) {
        m_manager->renameItem(jid, newContactName);
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Could not rename contact, as a result of not being connected."));
        qWarning() << "[client] [RosterManager] Could not rename contact, as a result of "
                      "not being connected.";
    }
}

void RosterManager::subscribeToPresence(const QString &contactJid)
{
    m_manager->subscribeTo(contactJid).then(this, [contactJid](QXmpp::SendResult result) {
        if (const auto error = std::get_if<QXmppError>(&result)) {
            Q_EMIT Kaidan::instance()->passiveNotificationRequested(
                tr("Requesting to see the personal data of %1 failed because of a connection problem: %2").arg(contactJid, error->description));
        }
    });
}

bool RosterManager::acceptSubscriptionToPresence(const QString &contactJid)
{
    if (m_manager->acceptSubscription(contactJid)) {
        m_unrespondedSubscriptionRequests.remove(contactJid);
        return true;
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Allowing %1 to see your personal data failed").arg(contactJid));
        return false;
    }
}

bool RosterManager::refuseSubscriptionToPresence(const QString &contactJid)
{
    if (m_manager->refuseSubscription(contactJid)) {
        m_unrespondedSubscriptionRequests.remove(contactJid);
        return true;
    } else {
        Q_EMIT Kaidan::instance()->passiveNotificationRequested(tr("Stopping %1 to see your personal data failed").arg(contactJid));
        return false;
    }
}

QMap<QString, QXmppPresence> RosterManager::unrespondedPresenceSubscriptionRequests()
{
    return m_unrespondedSubscriptionRequests;
}

void RosterManager::updateGroups(const QString &jid, const QString &name, const QList<QString> &groups)
{
    m_isItemBeingChanged = true;

    m_clientWorker->startTask([this, jid, name, groups] {
        // TODO: Add updating only groups to QXmppRosterManager without the need to pass the unmodified name
        m_manager->addItem(jid, name, QSet(groups.cbegin(), groups.cend()));
    });
}

void RosterManager::applyOldContactData(const QString &oldContactJid, const QString &newContactJid)
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

#include "moc_RosterManager.cpp"
