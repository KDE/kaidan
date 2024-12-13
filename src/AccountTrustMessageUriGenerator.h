// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include "TrustMessageUriGenerator.h"

class AccountTrustMessageUriGenerator : public TrustMessageUriGenerator
{
    Q_OBJECT

public:
    explicit AccountTrustMessageUriGenerator(QObject *parent = nullptr);

private:
    void setUp();
    void handleKeysChanged(const QString &accountJid, const QList<QString> &jids);
    void updateKeys();
};
