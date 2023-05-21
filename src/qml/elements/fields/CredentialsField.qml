// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

import im.kaidan.kaidan 1.0

/**
 * This is the base for fields which are used to enter credentials.
 */
Field {
	valid: false

	property alias credentialsValidator: credentialsValidator

	CredentialsValidator {
		id: credentialsValidator
	}
}
