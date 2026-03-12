// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AvatarCache.h"

// Qt
#include <QPixmap>
#include <QQuickItem>
#include <QQuickItemGrabResult>
// Kaidan
#include "RosterDb.h"

AvatarCache::AvatarCache(AccountSettings *accountSettings, QObject *parent)
    : QObject(parent)
    , m_accountSettings(accountSettings)
{
    connect(RosterDb::instance(), &RosterDb::itemRemoved, this, &AvatarCache::removeAvatar);
}

QPixmap AvatarCache::avatar(const QString &chatJid)
{
    return m_avatars.value(chatJid);
}

void AvatarCache::addAvatar(const QString &chatJid, QQuickItem *avatarItem)
{
    auto result = avatarItem->grabToImage();
    auto *resultPtr = result.get();

    connect(resultPtr, &QQuickItemGrabResult::ready, this, [this, chatJid, result, resultPtr]() mutable {
        m_avatars.insert(chatJid, QPixmap::fromImage(resultPtr->image()));
        result.reset();
    });
}

void AvatarCache::removeAvatar(const QString &accountJid, const QString &chatJid)
{
    if (m_accountSettings->jid() != accountJid) {
        return;
    }

    m_avatars.remove(chatJid);
}

#include "moc_AvatarCache.cpp"
