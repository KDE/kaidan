// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
#include <QMap>
#include <QObject>

class ClientWorker;
class QXmppClient;
class QXmppPresence;
class QXmppRosterManager;

class RosterController : public QObject
{
    Q_OBJECT

public:
    RosterController(QObject *parent = nullptr);

    /**
     * Adds a contact to the roster.
     *
     * @param nick name for the contact to be display
     * @param message message presented to the added contact
     * @param automaticInitialAddition whether the contact is added after a first received message
     */
    Q_INVOKABLE void addContact(const QString &jid, const QString &name = {}, const QString &message = {}, bool automaticInitialAddition = false);
    Q_SIGNAL void contactAdditionFailed(const QString &accountJid, const QString &jid);

    /**
     * Remove a contact from the roster.
     *
     * @param jid bare JID of the contact
     */
    Q_INVOKABLE void removeContact(const QString &jid);

    /**
     * Change a contact's name.
     *
     * @param jid bare JID of the contact
     * @param newContactName new name for the contact to be display
     */
    Q_INVOKABLE void renameContact(const QString &jid, const QString &newContactName);

    Q_INVOKABLE void subscribeToPresence(const QString &contactJid);
    Q_SIGNAL void presenceSubscriptionRequestReceived(const QString &accountJid, const QXmppPresence &request);
    QFuture<bool> acceptSubscriptionToPresence(const QString &contactJid);
    QFuture<bool> refuseSubscriptionToPresence(const QString &contactJid);
    QMap<QString, QXmppPresence> unrespondedPresenceSubscriptionRequests();

    Q_INVOKABLE void updateGroups(const QString &jid, const QString &name, const QList<QString> &groups = {});

private:
    void populateRoster();
    void handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
    void processSubscriptionRequestFromStranger(const QString &subscriberJid, const QXmppPresence &request);
    void addUnrespondedSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
    void applyOldContactData(const QString &oldContactJid, const QString &newContactJid);

    ClientWorker *const m_clientWorker;
    QXmppClient *const m_client;
    QXmppRosterManager *const m_manager;

    QMap<QString, QXmppPresence> m_unprocessedSubscriptionRequests;
    QMap<QString, QXmppPresence> m_pendingSubscriptionRequests;
    QMap<QString, QXmppPresence> m_unrespondedSubscriptionRequests;
    bool m_addingOwnJidToRosterAllowed = true;
    bool m_isItemBeingChanged = false;
};
