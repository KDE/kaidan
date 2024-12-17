// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountQrCodeGenerator.h"

AccountQrCodeGenerator::AccountQrCodeGenerator(QObject *parent)
    : AbstractQrCodeGenerator(parent)
{
    connect(this, &AccountQrCodeGenerator::jidChanged, this, &AccountQrCodeGenerator::updateUri);
    connect(&m_uriGenerator, &TrustMessageUriGenerator::uriChanged, this, &AccountQrCodeGenerator::updateText);
}

void AccountQrCodeGenerator::updateUri()
{
    m_uriGenerator.setJid(jid());
}

void AccountQrCodeGenerator::updateText()
{
    setText(m_uriGenerator.uri());
}

#include "moc_AccountQrCodeGenerator.cpp"
