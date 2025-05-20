// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "ContactTrustMessageUriGenerator.h"
#include "QrCodeGenerator.h"

/**
 * Gerenates a QR code encoding the Trust Message URI of a contact.
 *
 * If no keys for the Trust Message URI can be found, an XMPP URI containing only the contact's bare
 * JID is used.
 */
class ContactQrCodeGenerator : public QrCodeGenerator
{
    Q_OBJECT

    Q_PROPERTY(ContactTrustMessageUriGenerator *uriGenerator MEMBER m_uriGenerator WRITE setUriGenerator)

public:
    explicit ContactQrCodeGenerator(QObject *parent = nullptr);

    void setUriGenerator(ContactTrustMessageUriGenerator *uriGenerator);

private:
    void updateText();

    ContactTrustMessageUriGenerator *m_uriGenerator = nullptr;
};
