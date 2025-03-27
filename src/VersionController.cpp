// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "VersionController.h"

// QXmpp
#include <QXmppRosterManager.h>
#include <QXmppVersionManager.h>
// Kaidan
#include "FutureUtils.h"
#include "Globals.h"
#include "Kaidan.h"

VersionController::VersionController(QObject *parent)
    : QObject(parent)
    , m_rosterManager(Kaidan::instance()->client()->rosterManager())
    , m_versionManager(Kaidan::instance()->client()->versionManager())
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
        runOnThread(m_versionManager, [this, bareJid, resource, fetchVersion]() {
            const auto resources = m_rosterManager->getResources(bareJid);
            std::for_each(resources.cbegin(), resources.cend(), fetchVersion);
        });
    } else {
        runOnThread(m_versionManager, [resource, fetchVersion]() {
            fetchVersion(resource);
        });
    }
}

#include "moc_VersionController.cpp"
