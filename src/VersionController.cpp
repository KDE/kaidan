// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VersionController.h"

// QXmpp
#include <QXmppVersionManager.h>
// Kaidan
#include "Globals.h"
#include "PresenceCache.h"

VersionController::VersionController(PresenceCache *presenceCache, QXmppVersionManager *versionManager, QObject *parent)
    : QObject(parent)
    , m_presenceCache(presenceCache)
    , m_versionManager(versionManager)
{
    // Publish the own operating system and client information.
    m_versionManager->setClientName(QStringLiteral(APPLICATION_DISPLAY_NAME));
    m_versionManager->setClientVersion(QStringLiteral(VERSION_STRING));
    m_versionManager->setClientOs(QSysInfo::prettyProductName());

    connect(m_versionManager, &QXmppVersionManager::versionReceived, this, &VersionController::clientVersionReceived);
}

void VersionController::fetchVersions(const QString &bareJid, const QString &resource)
{
    const auto fetchVersion = [this, &bareJid](const QString &res) {
        m_versionManager->requestVersion(bareJid % u'/' % res);
    };

    if (resource.isEmpty()) {
        const auto resources = m_presenceCache->resources(bareJid);
        std::ranges::for_each(resources, fetchVersion);
    } else {
        fetchVersion(resource);
    }
}

#include "moc_VersionController.cpp"
