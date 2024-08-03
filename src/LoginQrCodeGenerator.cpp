// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "LoginQrCodeGenerator.h"

#include "qxmpp-exts/QXmppUri.h"

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
	QXmppUri uri;

	uri.setJid(jid());
	uri.setAction(QXmppUri::Login);

	if (Kaidan::instance()->settings()->authPasswordVisibility() != Kaidan::PasswordInvisible) {
		uri.setPassword(AccountManager::instance()->password());
	}

	setText(uri.toString());
}
