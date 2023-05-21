// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a password field with a hint for empty passwords and an option for showing the password.
 */
CredentialsField {
	labelText: qsTr("Password")
	invalidHintText: qsTr("Please enter a valid password")

	// indicator for showing the hidden password
	property bool showPassword: false

	// Add a button for showing and hiding the entered password.
	inputField {
		echoMode: showPassword ? TextInput.Normal : TextInput.Password
		rightActions: [
			Kirigami.Action {
				iconName: showPassword ? "password-show-on" : "password-show-off"
				onTriggered: showPassword = !showPassword
			}
		]
	}

	// Validate the entered password and show a hint if it is not valid.
	onTextChanged: {
		valid = credentialsValidator.isPasswordValid(text)
		toggleHintForInvalidText()
	}
}
