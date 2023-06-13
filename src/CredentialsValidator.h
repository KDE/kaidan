// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

#pragma once

#include <QObject>

/**
 * This is a validator for XMPP account credentials.
 */
class CredentialsValidator : public QObject
{
	Q_OBJECT

public:
	CredentialsValidator(QObject *parent = nullptr);

	/**
	 * Returns true if the given string is a valid JID of an XMPP user.
	 *
	 * @param jid JID to be validated
	 */
	Q_INVOKABLE static bool isUserJidValid(const QString &jid);

	/**
	 * Returns true if the given string is a valid username.
	 *
	 * @param username username to be validated
	 */
	Q_INVOKABLE static bool isUsernameValid(const QString &username);

	/**
	 * Returns true if the given string is a valid server.
	 *
	 * @param username server to be validated
	 */
	Q_INVOKABLE static bool isServerValid(const QString &server);

	/**
	 * Returns true if the given string is a valid password.
	 *
	 * @param password password to be validated
	 */
	Q_INVOKABLE static bool isPasswordValid(const QString &password);
};
