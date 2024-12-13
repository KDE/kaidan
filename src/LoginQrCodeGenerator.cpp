// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LoginQrCodeGenerator.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "AccountManager.h"
#include "Kaidan.h"
#include "Settings.h"

LoginQrCodeGenerator::LoginQrCodeGenerator(QObject *parent)
    : AbstractQrCodeGenerator(parent)
{
    connect(this, &LoginQrCodeGenerator::jidChanged, this, &LoginQrCodeGenerator::updateText);
    connect(AccountManager::instance(), &AccountManager::passwordChanged, this, &LoginQrCodeGenerator::updateText);
}

void LoginQrCodeGenerator::updateText()
{
    QXmpp::Uri::Login loginQuery;

    if (Kaidan::instance()->settings()->authPasswordVisibility() != Kaidan::PasswordInvisible) {
        loginQuery.password = (AccountManager::instance()->password());
    }

    QXmppUri uri;

    uri.setJid(jid());
    uri.setQuery(std::move(loginQuery));

    setText(uri.toString());
}
