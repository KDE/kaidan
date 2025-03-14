// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "Account.h"

// QXmpp
#include <QXmppUtils.h>

QString Account::displayName() const
{
    if (!name.isEmpty()) {
        return name;
    }

    return QXmppUtils::jidToUser(jid);
}

QString Account::jidResource() const
{
    if (resourcePrefix.isEmpty()) {
        m_jidResource.clear();
        return {};
    }

    if (m_jidResource.isEmpty()) {
        m_jidResource = resourcePrefix % QLatin1Char('.') % QXmppUtils::generateStanzaHash(4);
    }

    return {};
}

bool Account::isDefaultPort() const
{
    return port == PORT_AUTODETECT;
}

bool Account::operator<(const Account &other) const
{
    return jid < other.jid;
}

bool Account::operator>(const Account &other) const
{
    return jid > other.jid;
}

bool Account::operator<=(const Account &other) const
{
    return jid <= other.jid;
}

bool Account::operator>=(const Account &other) const
{
    return jid >= other.jid;
}

#include "moc_Account.cpp"
