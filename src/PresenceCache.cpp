// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "PresenceCache.h"
// Qt
// QXmpp
#include <QXmppUtils.h>

PresenceCache *PresenceCache::s_instance = nullptr;

QString Presence::availabilityToIcon(Availability type)
{
    switch (type) {
    case Online:
        return QStringLiteral("im-user-online");
    case Chat:
        return QStringLiteral("im-user-online");
    case Away:
        return QStringLiteral("im-user-away");
    case DND:
        return QStringLiteral("im-user-busy");
    case XA:
        return QStringLiteral("im-user-away");
    case Offline:
        return QStringLiteral("im-user-offline");
    }
    Q_UNREACHABLE();
    return {};
}

QString Presence::availabilityToText(Availability type)
{
    switch (type) {
    case Online:
        return QObject::tr("Available");
    case Chat:
        return QObject::tr("Free for chat");
    case Away:
        return QObject::tr("Away");
    case DND:
        return QObject::tr("Do not disturb");
    case XA:
        return QObject::tr("Away for longer");
    case Offline:
        return QObject::tr("Offline");
    }
    Q_UNREACHABLE();
    return {};
}

QColor Presence::availabilityToColor(Availability type)
{
    switch (type) {
    case Online:
        return QColorConstants::Svg::green;
    case Chat:
        return QColorConstants::Svg::darkgreen;
    case Away:
        return QColorConstants::Svg::orange;
    case DND:
        return QColor::fromRgb(218, 68, 83);
    case XA:
        return QColorConstants::Svg::orange;
    case Offline:
        return QColorConstants::Svg::silver;
    }
    Q_UNREACHABLE();
    return {};
}

PresenceCache::PresenceCache(QObject *parent)
    : QObject(parent)
{
    Q_ASSERT(!s_instance);
    s_instance = this;
}

PresenceCache::~PresenceCache()
{
    s_instance = nullptr;
}

QString PresenceCache::pickIdealResource(const QString &jid)
{
    if (!m_presences.contains(jid))
        return {};

    const auto &userPresences = m_presences[jid];
    if (userPresences.isEmpty())
        return {};
    if (userPresences.size() == 1)
        return userPresences.firstKey();

    auto result = userPresences.cbegin();
    for (auto itr = std::next(result); itr != userPresences.cend(); itr++) {
        if (presenceMoreImportant(*itr, *result))
            result = itr;
    }

    return result.key();
}

QList<QString> PresenceCache::resources(const QString &jid)
{
    if (!m_presences.contains(jid))
        return {};

    return m_presences.value(jid).keys();
}

/**
 * Returns the count of a JID's available resources.
 *
 * @param jid JID whose resources are counted
 * @return the resources count
 */
int PresenceCache::resourcesCount(const QString &jid)
{
    return m_presences.value(jid).size();
}

std::optional<QXmppPresence> PresenceCache::presence(const QString &jid, const QString &resource)
{
    if (const auto itr = m_presences.constFind(jid); itr != m_presences.cend()) {
        if (const auto resourceItr = itr->constFind(resource); resourceItr != itr->cend()) {
            return *resourceItr;
        }
    }
    return std::nullopt;
}

void PresenceCache::updatePresence(const QXmppPresence &presence)
{
    if (presence.type() != QXmppPresence::Available && presence.type() != QXmppPresence::Unavailable)
        return;

    const auto jid = QXmppUtils::jidToBareJid(presence.from());
    const auto resource = QXmppUtils::jidToResource(presence.from());

    if (!m_presences.contains(jid))
        m_presences.insert(jid, {});

    auto &userPresences = m_presences[jid];

    //
    // Presence updates can only go this way:
    //                  /---------------------------v
    // +-----------+   /   +---------+       +--------------+
    // | Connected | ----> | Updated | ----> | Disconnected |
    // +-----------+       +---------+  \    +--------------+
    //                          ^_______/
    //

    if (userPresences.contains(resource)) {
        if (presence.type() == QXmppPresence::Available) {
            m_presences[jid][resource] = presence;
            Q_EMIT presenceChanged(Updated, jid, resource);
        } else {
            // presence is 'Unavailable'
            userPresences.remove(resource);
            if (userPresences.isEmpty())
                m_presences.remove(jid);

            Q_EMIT presenceChanged(Disconnected, jid, resource);
        }
    } else {
        // client is unknown (hasn't been cached yet)
        if (presence.type() == QXmppPresence::Available) {
            userPresences.insert(resource, presence);
            Q_EMIT presenceChanged(Connected, jid, resource);
        }

        // presences from unknown clients that are unavailable are ignored
    }
}

void PresenceCache::clear()
{
    m_presences.clear();
    Q_EMIT presencesCleared();
}

constexpr qint8 PresenceCache::availabilityPriority(QXmppPresence::AvailableStatusType type)
{
    switch (type) {
    case QXmppPresence::XA:
    case QXmppPresence::Away:
        return 0;
    case QXmppPresence::Online:
        return 1;
    case QXmppPresence::Chat:
        return 2;
    case QXmppPresence::DND:
        return 3;
    default:
        return -1;
    }
}

bool PresenceCache::presenceMoreImportant(const QXmppPresence &a, const QXmppPresence &b)
{
    if (a.priority() != b.priority())
        return a.priority() > b.priority();

    if (const auto aAvailable = availabilityPriority(a.availableStatusType()), bAvailable = availabilityPriority(b.availableStatusType());
        aAvailable != bAvailable) {
        return aAvailable > bAvailable;
    }

    return !a.statusText().isEmpty() > !b.statusText().isEmpty();
}

UserResourcesWatcher::UserResourcesWatcher(QObject *parent)
    : QObject(parent)
{
    connect(PresenceCache::instance(), &PresenceCache::presenceChanged, this, [this](PresenceCache::ChangeType, const QString &, const QString &) {
        Q_EMIT resourcesCountChanged();
    });

    connect(PresenceCache::instance(), &PresenceCache::presencesCleared, this, &UserResourcesWatcher::resourcesCountChanged);
}

/**
 * Returns the JID whose resource changes are watched.
 *
 * @return the watched JID
 */
QString UserResourcesWatcher::jid() const
{
    return m_jid;
}

/**
 * Sets the JID whose resource changes are watched.
 *
 * @param jid JID to be watched
 */
void UserResourcesWatcher::setJid(const QString &jid)
{
    if (m_jid != jid) {
        m_jid = jid;
        Q_EMIT jidChanged();

        // That signal is emitted to reload the resources in QML after setting
        // the JID.
        Q_EMIT resourcesCountChanged();
    }
}

/**
 * Returns the count of available resources.
 *
 * @return the resources count
 */
int UserResourcesWatcher::resourcesCount()
{
    return PresenceCache::instance()->resourcesCount(m_jid);
}

#include "moc_PresenceCache.cpp"
