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
// Kaidan
#include "RosterItem.h"

class AccountSettings;
class Connection;
class EncryptionController;
class QXmppPresence;
class QXmppRosterManager;

class RosterController : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QList<QString> groups READ groups NOTIFY groupsChanged)

public:
    /**
     * Result for adding a contact with an XMPP URI specifying how the URI is used.
     */
    enum class ContactAdditionWithUriResult {
        AddingContact, ///< The contact is being added to the roster.
        ContactExists, ///< The contact is already in the roster.
        InvalidUri, ///< The URI cannot be used for contact addition.
    };
    Q_ENUM(ContactAdditionWithUriResult)

    RosterController(AccountSettings *accountSettings,
                     Connection *connection,
                     EncryptionController *encryptionController,
                     QXmppRosterManager *manager,
                     QObject *parent = nullptr);

    /**
     * Adds a contact (bare JID) with a given XMPP URI (e.g., from a scanned QR code) such as "xmpp:user@example.org".
     *
     * @param uriString XMPP URI string that contains only a JID
     */
    Q_INVOKABLE ContactAdditionWithUriResult addContactWithUri(const QString &uriString, const QString &name = {}, const QString &message = {});

    /**
     * Adds a contact to the roster.
     *
     * @param jid bare JID of the contact
     * @param name name for the contact to be display
     * @param message message presented to the added contact
     * @param automaticInitialAddition whether the contact is added after a first received message
     */
    Q_INVOKABLE void addContact(const QString &jid, const QString &name = {}, const QString &message = {}, bool automaticInitialAddition = false);
    Q_SIGNAL void contactAdditionFailed(const QString &jid);

    Q_INVOKABLE void renameContact(const QString &jid, const QString &newContactName);

    Q_INVOKABLE void subscribeToPresence(const QString &contactJid);
    Q_SIGNAL void presenceSubscriptionRequestReceived(const QXmppPresence &request);
    QFuture<bool> acceptSubscriptionToPresence(const QString &contactJid);
    Q_INVOKABLE QFuture<bool> refuseSubscriptionToPresence(const QString &contactJid);
    QMap<QString, QXmppPresence> unrespondedPresenceSubscriptionRequests();

    QList<QString> groups() const;
    Q_SIGNAL void groupsChanged();

    Q_INVOKABLE void updateGroup(const QString &oldGroup, const QString &newGroup);
    Q_INVOKABLE void removeGroup(const QString &group);
    Q_INVOKABLE void updateGroups(const QString &jid, const QString &name, const QList<QString> &groups = {});

    Q_INVOKABLE void setChatStateSendingEnabled(const QString &jid, bool chatStateSendingEnabled);
    Q_INVOKABLE void setReadMarkerSendingEnabled(const QString &jid, bool readMarkerSendingEnabled);
    Q_INVOKABLE void setNotificationRule(const QString &jid, RosterItem::NotificationRule notificationRule);
    Q_INVOKABLE void setAutomaticMediaDownloadsRule(const QString &jid, RosterItem::AutomaticMediaDownloadsRule rule);

    Q_INVOKABLE void removeContact(const QString &jid);

private:
    void populateRoster();
    void handleSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
    void processSubscriptionRequestFromStranger(const QString &subscriberJid, const QXmppPresence &request);
    void addUnrespondedSubscriptionRequest(const QString &subscriberJid, const QXmppPresence &request);
    void applyOldContactData(const QString &oldContactJid, const QString &newContactJid);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    EncryptionController *const m_encryptionController;
    QXmppRosterManager *const m_manager;

    QMap<QString, QXmppPresence> m_unprocessedSubscriptionRequests;
    QMap<QString, QXmppPresence> m_pendingSubscriptionRequests;
    QMap<QString, QXmppPresence> m_unrespondedSubscriptionRequests;
    QList<QString> m_pendingAutomaticInitialAdditionJids;
    bool m_addingOwnJidToRosterAllowed = true;
    bool m_isItemBeingChanged = false;
};

Q_DECLARE_METATYPE(RosterController::ContactAdditionWithUriResult)
