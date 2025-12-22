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
#include <QXmppUri.h>
#include <QXmppUtils.h>
// Kaidan
#include "Account.h"
#include "ClientWorker.h"
#include "EncryptionController.h"
#include "KaidanCoreLog.h"
#include "MainController.h"
#include "MessageDb.h"
#include "RosterDb.h"
#include "RosterModel.h"

RosterController::RosterController(AccountSettings *accountSettings,
                                   Connection *connection,
                                   EncryptionController *encryptionController,
                                   QXmppRosterManager *manager,
                                   QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
    , m_connection(connection)
    , m_encryptionController(encryptionController)
    , m_manager(manager)
{
    connect(m_manager, &QXmppRosterManager::rosterReceived, this, &RosterController::populateRoster);

    connect(m_manager, &QXmppRosterManager::itemAdded, this, [this](const QString &jid) {
        runOnThread(
            m_manager,
            [this, jid]() {
                return m_manager->getRosterEntry(jid);
            },
            this,
            [this, jid](QXmppRosterIq::Item &&addedItem) {
                RosterItem rosterItem{m_accountSettings->jid(), addedItem};
                rosterItem.encryption = m_accountSettings->encryption();
                rosterItem.lastMessageDateTime = QDateTime::currentDateTimeUtc();
                RosterDb::instance()->addItem(rosterItem);

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
                RosterDb::instance()->updateItem(m_accountSettings->jid(), jid, [jid, changedItem](RosterItem &item) {
                    item.name = changedItem.name();
                    item.subscription = changedItem.subscriptionType();

                    const auto groups = changedItem.groups();
                    item.groups = {groups.cbegin(), groups.cend()};
                });

                if (m_isItemBeingChanged) {
                    m_isItemBeingChanged = false;
                }
            });
    });

    connect(m_manager, &QXmppRosterManager::itemRemoved, this, [this](const QString &jid) {
        const auto accountJid = m_accountSettings->jid();
        MessageDb::instance()->removeMessages(accountJid, jid);
        RosterDb::instance()->removeItem(accountJid, jid);

        // Do not remove own devices in case the notes chat is removed.
        if (jid != accountJid) {
            m_encryptionController->removeContactDevices(jid);
        }
    });

    connect(m_manager, &QXmppRosterManager::subscriptionRequestReceived, this, &RosterController::handleSubscriptionRequest);

    connect(RosterModel::instance(), &RosterModel::itemAdded, this, [this](const RosterItem &item) {
        // MessageController checks whether RosterModel contains a roster item in order to add a missing item to the roster.
        // Thus, it cannot be removed from m_automaticInitialAdditionJids directly after the item is added to the roster.
        m_pendingAutomaticInitialAdditionJids.removeOne(item.jid);
    });

    connect(RosterModel::instance(), &RosterModel::groupsChanged, this, &RosterController::groupsChanged);
}

RosterController::ContactAdditionWithUriResult RosterController::addContactWithUri(const QString &uriString, const QString &name, const QString &message)
{
    if (const auto uriParsingResult = QXmppUri::fromString(uriString); std::holds_alternative<QXmppUri>(uriParsingResult)) {
        const auto uri = std::get<QXmppUri>(uriParsingResult);
        const auto jid = uri.jid();

        if (jid.isEmpty()) {
            return ContactAdditionWithUriResult::InvalidUri;
        }

        if (const auto accountJid = m_accountSettings->jid(); RosterModel::instance()->hasItem(accountJid, jid)) {
            Q_EMIT MainController::instance()->openChatPageRequested(accountJid, jid);
            return ContactAdditionWithUriResult::ContactExists;
        }

        addContact(jid, name, message);

        return ContactAdditionWithUriResult::AddingContact;
    }

    return ContactAdditionWithUriResult::InvalidUri;
}

void RosterController::addContact(const QString &jid, const QString &name, const QString &message, bool automaticInitialAddition)
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        if (automaticInitialAddition) {
            if (m_pendingAutomaticInitialAdditionJids.contains(jid)) {
                return;
            }

            m_pendingAutomaticInitialAdditionJids.append(jid);
        }

        // Do not try to add the own JID to the roster multiple times if the server does not support it.
        if (const auto ownJidBeingAdded = jid == m_accountSettings->jid(); ownJidBeingAdded && !m_addingOwnJidToRosterAllowed) {
            if (!automaticInitialAddition) {
                Q_EMIT contactAdditionFailed(jid);
            }
        } else {
            callRemoteTask(
                m_manager,
                [this, jid, name]() {
                    return std::pair{m_manager->addRosterItem(jid, name), this};
                },
                this,
                [this, jid, message, automaticInitialAddition, ownJidBeingAdded](QXmppRosterManager::Result &&result) {
                    if (const auto error = std::get_if<QXmppError>(&result)) {
                        if (const auto stanzaError = error->takeValue<QXmppStanza::Error>(); ownJidBeingAdded && stanzaError
                            && stanzaError->type() == QXmppStanza::Error::Cancel && stanzaError->condition() == QXmppStanza::Error::NotAllowed) {
                            m_addingOwnJidToRosterAllowed = false;

                            if (automaticInitialAddition) {
                                return;
                            }

                            m_pendingAutomaticInitialAdditionJids.removeOne(jid);
                            Q_EMIT contactAdditionFailed(jid);

                            return;
                        }

                        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("%1 could not be added: %2").arg(jid, error->description));
                    } else if (!ownJidBeingAdded && !QXmppUtils::jidToUser(jid).isEmpty()) {
                        runOnThread(m_manager, [this, jid, message]() {
                            m_manager->subscribeTo(jid, message);
                        });
                    }
                });
        }
    } else {
        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not add contact as a result of not being connected."));
        qCWarning(KAIDAN_CORE_LOG) << "Could not add contact as a result of not being connected.";
    }
}

void RosterController::renameContact(const QString &jid, const QString &newContactName)
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this, jid, newContactName]() {
            m_manager->renameItem(jid, newContactName);
        });
    } else {
        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not rename contact as a result of not being connected."));
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
                Q_EMIT MainController::instance()->passiveNotificationRequested(
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
                Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Allowing %1 to see your personal data failed").arg(contactJid));
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
                Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Stopping %1 to see your personal data failed").arg(contactJid));
                return false;
            }
        });
}

QMap<QString, QXmppPresence> RosterController::unrespondedPresenceSubscriptionRequests()
{
    return m_unrespondedSubscriptionRequests;
}

QList<QString> RosterController::groups() const
{
    const auto &items = RosterModel::instance()->items(m_accountSettings->jid());
    QList<QString> groups;

    std::ranges::for_each(items, [&groups](const RosterItem &item) {
        std::for_each(item.groups.cbegin(), item.groups.cend(), [&](const QString &group) {
            if (!groups.contains(group)) {
                groups.append(group);
            }
        });
    });

    std::sort(groups.begin(), groups.end());

    return groups;
}

void RosterController::updateGroup(const QString &oldGroup, const QString &newGroup)
{
    const auto &items = RosterModel::instance()->items(m_accountSettings->jid());

    for (const auto &item : items) {
        if (auto groups = item.groups; groups.contains(oldGroup)) {
            groups.removeOne(oldGroup);
            groups.append(newGroup);

            updateGroups(item.jid, item.name, groups);
        }
    }
}

void RosterController::removeGroup(const QString &group)
{
    const auto &items = RosterModel::instance()->items(m_accountSettings->jid());

    for (const auto &item : items) {
        if (auto groups = item.groups; groups.contains(group)) {
            groups.removeOne(group);

            updateGroups(item.jid, item.name, groups);
        }
    }
}

void RosterController::updateGroups(const QString &jid, const QString &name, const QList<QString> &groups)
{
    m_isItemBeingChanged = true;

    runOnThread(m_manager, [this, jid, name, groups]() {
        // TODO: Add updating only groups to QXmppRosterManager without the need to pass the unmodified name
        m_manager->addItem(jid, name, {groups.cbegin(), groups.cend()});
    });
}

void RosterController::setChatStateSendingEnabled(const QString &jid, bool chatStateSendingEnabled)
{
    RosterDb::instance()->updateItem(m_accountSettings->jid(), jid, [=](RosterItem &item) {
        item.chatStateSendingEnabled = chatStateSendingEnabled;
    });
}

void RosterController::setReadMarkerSendingEnabled(const QString &jid, bool readMarkerSendingEnabled)
{
    RosterDb::instance()->updateItem(m_accountSettings->jid(), jid, [=](RosterItem &item) {
        item.readMarkerSendingEnabled = readMarkerSendingEnabled;
    });
}

void RosterController::setNotificationRule(const QString &jid, RosterItem::NotificationRule notificationRule)
{
    RosterDb::instance()->updateItem(m_accountSettings->jid(), jid, [=](RosterItem &item) {
        item.notificationRule = notificationRule;
    });
}

void RosterController::setAutomaticMediaDownloadsRule(const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule)
{
    RosterDb::instance()->updateItem(m_accountSettings->jid(), jid, [rule](RosterItem &item) {
        item.automaticMediaDownloadsRule = rule;
    });
}

void RosterController::removeContact(const QString &jid)
{
    if (m_connection->state() == Enums::ConnectionState::StateConnected) {
        runOnThread(m_manager, [this, jid]() {
            m_manager->removeItem(jid);
        });
    } else {
        Q_EMIT MainController::instance()->passiveNotificationRequested(tr("Could not remove contact as a result of not being connected."));
        qCWarning(KAIDAN_CORE_LOG) << "Could not remove contact as a result of not being connected.";
    }
}

void RosterController::populateRoster()
{
    qCDebug(KAIDAN_CORE_LOG) << "Populating roster";

    const auto accountJid = m_accountSettings->jid();

    runOnThread(
        m_manager,
        [this, accountJid, encryption = m_accountSettings->encryption()]() {
            QList<RosterItem> rosterItems;
            const auto jids = m_manager->getRosterBareJids();

            for (const auto &jid : jids) {
                RosterItem rosterItem = {accountJid, m_manager->getRosterEntry(jid)};
                rosterItem.encryption = encryption;
                rosterItems.append(rosterItem);
            }

            return rosterItems;
        },
        this,
        [this, accountJid](QList<RosterItem> &&rosterItems) {
            for (auto itr = rosterItems.begin(); itr != rosterItems.end(); ++itr) {
                const auto jid = itr->jid;

                // Process subscription requests from roster items that were received before the roster was
                // received.
                if (m_unprocessedSubscriptionRequests.contains(jid)) {
                    addUnrespondedSubscriptionRequest(jid, m_unprocessedSubscriptionRequests.take(jid));
                }
            }

            // replace current contacts with new ones from server
            RosterDb::instance()->replaceItems(accountJid, rosterItems);

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
    Q_EMIT presenceSubscriptionRequestReceived(request);
}

void RosterController::applyOldContactData(const QString &oldContactJid, const QString &newContactJid)
{
    if (oldContactJid.isEmpty()) {
        return;
    }

    const auto accountJid = m_accountSettings->jid();

    if (const auto oldItem = RosterModel::instance()->item(accountJid, oldContactJid)) {
        RosterDb::instance()->updateItem(accountJid, newContactJid, [oldItem = *oldItem](RosterItem &newItem) {
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
