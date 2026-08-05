// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Qt
#include <QObject>
#include <QtQml/qqmlregistration.h>

class PresenceCache;
class QXmppClient;
class QXmppRosterManager;
class QXmppVersionIq;
class QXmppVersionManager;

Q_MOC_INCLUDE("QXmppVersionIq.h")

class VersionController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Not creatable from QML")

public:
    VersionController(PresenceCache *presenceCache, QXmppVersionManager *versionManager, QObject *parent = nullptr);

    /**
     * Fetches the version information of all resources of the given  bare JID
     */
    Q_INVOKABLE void fetchVersions(const QString &bareJid, const QString &resource);

    /**
     * Emitted when a client version information was received
     */
    Q_SIGNAL void clientVersionReceived(const QXmppVersionIq &versionIq);

private:
    PresenceCache *const m_presenceCache;
    QXmppVersionManager *const m_versionManager;
};
