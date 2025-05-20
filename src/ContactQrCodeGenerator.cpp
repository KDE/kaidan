// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "ContactQrCodeGenerator.h"

ContactQrCodeGenerator::ContactQrCodeGenerator(QObject *parent)
    : QrCodeGenerator(parent)
{
}

void ContactQrCodeGenerator::setUriGenerator(ContactTrustMessageUriGenerator *uriGenerator)
{
    if (m_uriGenerator != uriGenerator) {
        m_uriGenerator = uriGenerator;
        connect(m_uriGenerator, &ContactTrustMessageUriGenerator::uriChanged, this, &ContactQrCodeGenerator::updateText);
    }
}

void ContactQrCodeGenerator::updateText()
{
    setText(m_uriGenerator->uri());
}

#include "moc_ContactQrCodeGenerator.cpp"
