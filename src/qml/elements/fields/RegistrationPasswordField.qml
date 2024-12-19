// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

/**
 * This is a password field for registration with an option for showing the password and extras regarding registration.
 * The password strength is indicated by a colored bar.
 * A hint is shown for improving the password strength.
 */
PasswordField {
	valid: true

	placeholderText: {
		if (inputField.echoMode === TextInput.Password) {
			return "●".repeat(generatedPassword.length)
		}

		return generatedPassword
	}

	property string generatedPassword: credentialsGenerator.generatePassword()
}
