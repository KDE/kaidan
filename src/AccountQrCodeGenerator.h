// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "AbstractQrCodeGenerator.h"
#include "AccountTrustMessageUriGenerator.h"

/**
 * Gerenates a QR code encoding the Trust Message URI of an account.
 *
 * If no keys for the Trust Message URI can be found, an XMPP URI containing only the account's bare
 * JID is used.
 */
class AccountQrCodeGenerator : public AbstractQrCodeGenerator
{
    Q_OBJECT

public:
    explicit AccountQrCodeGenerator(QObject *parent = nullptr);

private:
    void updateUri();
    void updateText();

    AccountTrustMessageUriGenerator m_uriGenerator;
};
