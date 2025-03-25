// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LoginQrCodeGenerator.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "AccountController.h"
#include "Kaidan.h"

LoginQrCodeGenerator::LoginQrCodeGenerator(QObject *parent)
    : AbstractQrCodeGenerator(parent)
{
    connect(this, &LoginQrCodeGenerator::jidChanged, this, &LoginQrCodeGenerator::updateText);
    connect(AccountController::instance(), &AccountController::accountChanged, this, &LoginQrCodeGenerator::updateText);
}

void LoginQrCodeGenerator::updateText()
{
    const auto accountController = AccountController::instance();
    QXmpp::Uri::Login loginQuery;

    if (accountController->account().passwordVisibility != Kaidan::PasswordInvisible) {
        loginQuery.password = accountController->account().password;
    }

    QXmppUri uri;

    uri.setJid(jid());
    uri.setQuery(std::move(loginQuery));

    setText(uri.toString());
}

#include "moc_LoginQrCodeGenerator.cpp"
