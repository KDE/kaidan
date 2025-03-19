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

class OmemoController : public QObject
{
    Q_OBJECT

public:
    explicit OmemoController(QObject *parent = nullptr);

    QFuture<void> load();
    QFuture<void> setUp();
    QFuture<void> unload();
    QFuture<void> reset();
    QFuture<void> resetLocally();

    QFuture<void> initializeAccount(const QString &accountJid);
    QFuture<void> initializeChat(const QString &accountJid, const QList<QString> &jids);

    QFuture<bool> hasUsableDevices(const QList<QString> &jids);

    QFuture<void> requestDeviceLists(const QList<QString> &jids);
    QFuture<void> subscribeToDeviceLists(const QList<QString> &jids);

    QFuture<QString> ownKey(const QString &accountJid);
    QFuture<QHash<QString, QHash<QString, QXmpp::TrustLevel>>> keys(const QString &accountJid, const QList<QString> &jids, QXmpp::TrustLevels trustLevels = {});

    QFuture<EncryptionController::OwnDevice> ownDevice(const QString &accountJid);
    QFuture<QList<EncryptionController::Device>> devices(const QString &accountJid, const QList<QString> &jids);

    void removeContactDevices(const QString &jid);

private:
    void buildMissingSessions(QFutureInterface<void> &interface, const QList<QString> &jids);

    /**
     * Enables session building for new devices even before sending a message.
     */
    void enableSessionBuildingForNewDevices();

    QFuture<void> unsubscribeFromDeviceLists();

    bool m_isLoaded = false;
};
