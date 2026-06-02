// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan
import ".."

/**
 * This is a password field with a hint for empty passwords and an option for showing the password.
 */
FormCard.FormPasswordFieldDelegate {
	id: root

	property alias completionRole: completer.role
	property alias completionModel: completer.model
	readonly property alias input: completer.input
	property string invalidHintText: qsTr("Enter a valid password")
	property bool valid: acceptableInput
	property bool forceHintTextVisible: false
	readonly property InputValidator inputValidator: InputValidator {
		patterns: InputValidator.Pattern.NotEmpty
	}
	readonly property bool hintTextVisible: forceHintTextVisible || ((text.length > 0 || fieldActiveFocus) && inputValidator.state === InputValidator.Invalid)

	label: qsTr("Password")
	status: Kirigami.MessageType.Warning
	statusMessage: hintTextVisible ? invalidHintText : ""
	validator: inputValidator
	// Disable predictive text input to make textEdited() work under Android in completer.
	inputMethodHints: Qt.ImhSensitiveData
	// TODO: Uncomment the following line once Kirigami Addons > v1.12.1 is required by Kaidan.
	// selectByMouse: !Kirigami.Settings.isMobile
	onTextEdited: forceHintTextVisible = false
	Keys.onPressed: event => {
		switch (event.key) {
			case Qt.Key_Return:
			case Qt.Key_Enter:
				if (!valid)  {
					forceHintTextVisible = true
				}
		}
	}

	TextCompleter {
		id: completer
		textControl: root
	}
}
