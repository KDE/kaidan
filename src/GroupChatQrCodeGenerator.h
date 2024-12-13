// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "AbstractQrCodeGenerator.h"

/**
 * Gerenates a QR code encoding the address of a group chat to join it.
 */
class GroupChatQrCodeGenerator : public AbstractQrCodeGenerator
{
    Q_OBJECT

public:
    explicit GroupChatQrCodeGenerator(QObject *parent = nullptr);

private:
    void updateText();
};
