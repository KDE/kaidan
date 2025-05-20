// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "GroupChatQrCodeGenerator.h"

// QXmpp
#include <QXmppUri.h>

GroupChatQrCodeGenerator::GroupChatQrCodeGenerator(QObject *parent)
    : QrCodeGenerator(parent)
{
}

void GroupChatQrCodeGenerator::setJid(const QString &jid)
{
    if (m_jid != jid) {
        m_jid = jid;
        updateText();
    }
}

void GroupChatQrCodeGenerator::updateText()
{
    QXmppUri uri;

    uri.setJid(m_jid);
    uri.setQuery(QXmpp::Uri::Join());

    setText(uri.toString());
}

#include "moc_GroupChatQrCodeGenerator.cpp"
