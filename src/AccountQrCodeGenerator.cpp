// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "AccountQrCodeGenerator.h"

AccountQrCodeGenerator::AccountQrCodeGenerator(QObject *parent)
    : QrCodeGenerator(parent)
{
}

void AccountQrCodeGenerator::setUriGenerator(AccountTrustMessageUriGenerator *uriGenerator)
{
    if (m_uriGenerator != uriGenerator) {
        m_uriGenerator = uriGenerator;
        connect(m_uriGenerator, &AccountTrustMessageUriGenerator::uriChanged, this, &AccountQrCodeGenerator::updateText);
    }
}

void AccountQrCodeGenerator::updateText()
{
    setText(m_uriGenerator->uri());
}

#include "moc_AccountQrCodeGenerator.cpp"
