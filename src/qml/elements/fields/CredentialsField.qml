// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

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
