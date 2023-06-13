// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2021 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#include "CredentialsValidator.h"

#include <QXmppUtils.h>

CredentialsValidator::CredentialsValidator(QObject *parent)
        : QObject(parent)
{
}

bool CredentialsValidator::isUserJidValid(const QString &jid)
{
	return jid.count('@') == 1 && isUsernameValid(QXmppUtils::jidToUser(jid)) && isServerValid(QXmppUtils::jidToDomain(jid));
}

bool CredentialsValidator::isUsernameValid(const QString &username)
{
	return !(username.isEmpty() || username.contains(u' '));
}

bool CredentialsValidator::isServerValid(const QString &server)
{
	return !(server.isEmpty() || server.contains(u' '));
}

bool CredentialsValidator::isPasswordValid(const QString &password)
{
	return !password.isEmpty();
}
