// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "QrCodeGenerator.h"

class AccountSettings;

/**
 * Gerenates a QR code encoding the credentials of an account used to log into it with another
 * client.
 */
class LoginQrCodeGenerator : public QrCodeGenerator
{
    Q_OBJECT

    Q_PROPERTY(AccountSettings *accountSettings MEMBER m_accountSettings WRITE setAccountSettings)

public:
    explicit LoginQrCodeGenerator(QObject *parent = nullptr);

    void setAccountSettings(AccountSettings *accountSettings);

private:
    void updateText();

    AccountSettings *m_accountSettings = nullptr;
};
