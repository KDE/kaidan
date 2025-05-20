// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LoginQrCodeGenerator.h"

// QXmpp
#include <QXmppUri.h>
// Kaidan
#include "Account.h"

LoginQrCodeGenerator::LoginQrCodeGenerator(QObject *parent)
    : QrCodeGenerator(parent)
{
}

void LoginQrCodeGenerator::setAccountSettings(AccountSettings *accountSettings)
{
    if (m_accountSettings != accountSettings) {
        m_accountSettings = accountSettings;
        connect(m_accountSettings, &AccountSettings::passwordChanged, this, &LoginQrCodeGenerator::updateText);
        updateText();
    }
}

void LoginQrCodeGenerator::updateText()
{
    QXmpp::Uri::Login loginQuery;

    if (m_accountSettings->passwordVisibility() != AccountSettings::PasswordVisibility::Invisible) {
        loginQuery.password = m_accountSettings->password();
    }

    QXmppUri uri;

    uri.setJid(m_accountSettings->jid());
    uri.setQuery(std::move(loginQuery));

    setText(uri.toString());
}

#include "moc_LoginQrCodeGenerator.cpp"
