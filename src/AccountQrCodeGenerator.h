// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "AccountTrustMessageUriGenerator.h"
#include "QrCodeGenerator.h"

/**
 * Gerenates a QR code encoding the Trust Message URI of an account.
 *
 * If no keys for the Trust Message URI can be found, an XMPP URI containing only the account's bare
 * JID is used.
 */
class AccountQrCodeGenerator : public QrCodeGenerator
{
    Q_OBJECT

    Q_PROPERTY(AccountTrustMessageUriGenerator *uriGenerator MEMBER m_uriGenerator WRITE setUriGenerator)

public:
    explicit AccountQrCodeGenerator(QObject *parent = nullptr);

    void setUriGenerator(AccountTrustMessageUriGenerator *uriGenerator);

private:
    void updateText();

    AccountTrustMessageUriGenerator *m_uriGenerator = nullptr;
};
