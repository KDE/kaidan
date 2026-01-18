// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatroxde@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QImage>
#include <QObject>
// QXmpp
#include <QXmppVCardIq.h>

struct RosterItem;

class AccountSettings;
class ClientController;
class Connection;
class PresenceCache;
class QXmppClient;
class QXmppPresence;
class QXmppVCardManager;

class VCardController : public QObject
{
    Q_OBJECT

public:
    VCardController(AccountSettings *accountSettings,
                    Connection *connection,
                    PresenceCache *presenceCache,
                    QXmppClient *client,
                    QXmppVCardManager *vCardManager,
                    QObject *parent = nullptr);

    /**
     * Requests the vCard of a given JID from the JID's server.
     *
     * @param jid JID for which the vCard is being requested
     */
    void requestVCard(const QString &jid);

    /**
     * Requests the user's vCard from the server.
     */
    Q_INVOKABLE void requestOwnVCard();

    Q_SIGNAL void vCardReceived(const QXmppVCardIq &vCard);

    void updateOwnVCard(const QXmppVCardIq &vCard);

    /**
     * Changes the user's nickname.
     *
     * @param nickname name that is shown to contacts after the update
     */
    Q_INVOKABLE void changeNickname(const QString &nickname);

    Q_INVOKABLE void changeAvatar(const QImage &avatar = {});
    Q_SIGNAL void avatarChangeSucceeded();

private:
    void handleRosterItemsFetched(const QList<RosterItem> &items);
    void handleRosterItemAdded(const RosterItem &rosterItem);

    /**
     * Requests the vCard of an unavailable contact.
     *
     * Available contacts are already covered by handlePresenceReceived().
     */
    void requestContactVCard(const QString &accountJid, const QString &jid, bool isGroupChat);

    /**
     * Handles an incoming vCard and processes it like saving a containing user avatar etc..
     *
     * @param vCard IQ stanza containing the vCard
     */
    void handleVCardReceived(const QXmppVCardIq &vCard);

    /**
     * Handles the receiving of the user's vCard.
     */
    void handleOwnVCardReceived();

    /**
     * Handles an incoming presence stanza and checks if the user avatar needs to be refreshed.
     *
     * @param presence a contact's presence stanza
     */
    void handlePresenceReceived(const QXmppPresence &presence);

    /**
     * Changes the nickname which was cached to be set after receiving the current vCard.
     */
    void changeNicknameAfterReceivingCurrentVCard();

    /**
     * Changes the avatar which was cached to be set after receiving the current vCard.
     */
    void changeAvatarAfterReceivingCurrentVCard();

    void addAvatar(const QXmppVCardIq &vCard);

    AccountSettings *const m_accountSettings;
    Connection *const m_connection;
    PresenceCache *const m_presenceCache;
    QXmppVCardManager *const m_manager;

    QString m_nicknameToBeSetAfterReceivingCurrentVCard;
    QImage m_avatarToBeSetAfterReceivingCurrentVCard;
    bool m_isAvatarToBeReset = false;
};
