// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "AbstractQrCodeGenerator.h"

/**
 * Gerenates a QR code encoding the credentials of an account used to log into it with another
 * client.
 */
class LoginQrCodeGenerator : public AbstractQrCodeGenerator
{
    Q_OBJECT

public:
    explicit LoginQrCodeGenerator(QObject *parent = nullptr);

private:
    void updateText();
};
