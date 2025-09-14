// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a password field with a hint for empty passwords and an option for showing the password.
 */
CredentialsField {
	id: root

	// indicator for showing the hidden password
	property bool passwordShown: false

	labelText: qsTr("Password")
	inputField {
		echoMode: passwordShown ? TextInput.Normal : TextInput.Password
		rightActions: [
			Kirigami.Action {
				icon.name: root.passwordShown ? "password-show-on" : "password-show-off"
				onTriggered: root.passwordShown = !root.passwordShown
			}
		]
	}
	invalidHintText: qsTr("Please enter a valid password")
	valid: credentialsValidator.isPasswordValid(text)
}
