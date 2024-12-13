// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ContactQrCodeGenerator.h"

#include <QUrl>

ContactQrCodeGenerator::ContactQrCodeGenerator(QObject *parent)
    : AbstractQrCodeGenerator(parent)
{
    connect(this, &ContactQrCodeGenerator::jidChanged, this, &ContactQrCodeGenerator::updateUriJid);
    connect(&m_uriGenerator, &TrustMessageUriGenerator::uriChanged, this, &ContactQrCodeGenerator::updateText);
}

QString ContactQrCodeGenerator::accountJid() const
{
    return m_accountJid;
}

void ContactQrCodeGenerator::setAccountJid(const QString &accountJid)
{
    if (m_accountJid != accountJid) {
        m_accountJid = accountJid;
        Q_EMIT accountJidChanged();
        updateUriAccountJid();
    }
}

void ContactQrCodeGenerator::updateUriAccountJid()
{
    m_uriGenerator.setAccountJid(m_accountJid);
}

void ContactQrCodeGenerator::updateUriJid()
{
    m_uriGenerator.setJid(jid());
}

void ContactQrCodeGenerator::updateText()
{
    setText(m_uriGenerator.uri());
}
