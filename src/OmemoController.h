// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QFuture>
// QXmpp
#include <QXmppTask.h>
#include <QXmppTrustLevel.h>
// Kaidan
#include "EncryptionController.h"

class AccountSettings;
class PresenceCache;
class QXmppOmemoManager;

class OmemoController : public QObject
{
    Q_OBJECT

public:
    OmemoController(AccountSettings *accountSettings,
                    EncryptionController *encryptionController,
                    PresenceCache *presenceCache,
                    QXmppOmemoManager *manager,
                    QObject *parent = nullptr);

    QFuture<void> load();
    QFuture<void> setUp();
    QFuture<void> unload();
    QFuture<void> reset();
    QFuture<void> resetLocally();

    void initializeAccount();
    void initializeChat(const QList<QString> &jids);

    QFuture<bool> hasUsableDevices(const QList<QString> &jids);

    QFuture<void> requestDeviceLists(const QList<QString> &jids);
    QFuture<void> subscribeToDeviceLists(const QList<QString> &jids);

    QFuture<QString> ownKey();
    QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> keys(const QList<QString> &jids, QXmpp::TrustLevels trustLevels = {});

    QFuture<EncryptionController::OwnDevice> ownDevice();
    QFuture<QList<EncryptionController::Device>> devices(const QList<QString> &jids);

    void removeContactDevices(const QString &jid);

private:
    void buildMissingSessions(const QList<QString> &jids);

    /**
     * Enables session building for new devices even before sending a message.
     */
    void enableSessionBuildingForNewDevices();

    QFuture<void> unsubscribeFromDeviceLists();

    AccountSettings *const m_accountSettings;
    PresenceCache *const m_presenceCache;
    QXmppOmemoManager *const m_manager;

    bool m_isLoaded = false;
};
