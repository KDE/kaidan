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

class AvatarFileStorage;
class ClientWorker;
class QXmppClient;
class QXmppPresence;
class QXmppVCardIq;
class QXmppVCardManager;

class VCardController : public QObject
{
    Q_OBJECT

public:
    VCardController(QObject *parent = nullptr);

    /**
     * Requests the vCard of a given JID from the JID's server.
     *
     * @param jid JID for which the vCard is being requested
     */
    void requestVCard(const QString &jid);
    Q_SIGNAL void vCardReceived(const QXmppVCardIq &vCard);

    /**
     * Requests the user's vCard from the server.
     */
    Q_INVOKABLE void requestClientVCard();

    /**
     * Changes the user's nickname.
     *
     * @param nickname name that is shown to contacts after the update
     */
    Q_INVOKABLE void changeNickname(const QString &nickname);

    Q_INVOKABLE void changeAvatar(const QImage &avatar = {});
    Q_SIGNAL void avatarChangeSucceeded();

private:
    /**
     * Handles an incoming vCard and processes it like saving a containing user avatar etc..
     *
     * @param iq IQ stanza containing the vCard
     */
    void handleVCardReceived(const QXmppVCardIq &iq);

    /**
     * Handles the receiving of the user's vCard.
     */
    void handleClientVCardReceived();

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

    ClientWorker *const m_clientWorker;
    QXmppClient *const m_client;
    QXmppVCardManager *const m_manager;
    AvatarFileStorage *const m_avatarStorage;
    QString m_nicknameToBeSetAfterReceivingCurrentVCard;
    QImage m_avatarToBeSetAfterReceivingCurrentVCard;
    bool m_isAvatarToBeReset = false;
};
