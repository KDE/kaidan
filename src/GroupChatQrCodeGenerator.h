// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

// Kaidan
#include "QrCodeGenerator.h"

/**
 * Generates a QR code encoding the address of a group chat to join it.
 */
class GroupChatQrCodeGenerator : public QrCodeGenerator
{
    Q_OBJECT

    Q_PROPERTY(QString jid MEMBER m_jid WRITE setJid)

public:
    explicit GroupChatQrCodeGenerator(QObject *parent = nullptr);

    void setJid(const QString &jid);

private:
    void updateText();

    QString m_jid;
};
