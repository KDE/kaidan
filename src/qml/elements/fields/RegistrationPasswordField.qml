// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import im.kaidan.kaidan 1.0

/**
 * This is a password field for registration with an option for showing the password and extras regarding registration.
 * The password strength is indicated by a colored bar.
 * A hint is shown for improving the password strength.
 */
PasswordField {
	valid: true

	placeholderText: {
		if (field.inputField.echoMode === TextInput.Password)
			return "●".repeat(generatedPassword.length)
		return generatedPassword
	}

	property string generatedPassword: credentialsGenerator.generatePassword()
}
