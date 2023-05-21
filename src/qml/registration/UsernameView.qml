// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls

import im.kaidan.kaidan 1.0

import "../elements/fields"

/**
 * This view is used for entering a username.
 */
FieldView {
	descriptionText: qsTr("Your username is used to log in to your account and to chat with you.\nIf you don't enter a username, the randomly generated and already displayed one is used.\nUsing an own username may let it be more recognizable but also could decrease your privacy!")
	imageSource: "username"

	property string text: enteredText.length > 0 ? enteredText : field.placeholderText

	property alias enteredText: field.text
	property alias valid: field.valid

	function regenerateUsername() {
		placeholderText = credentialsGenerator.generateUsername()
	}

	ColumnLayout {
		parent: contentArea

		CredentialsField {
			id: field
			labelText: qsTr("Username")

			// Set the display name as the entered text while replacing all whitespaces by dots.
			text: displayNameView.text.replace(/ /g, ".").toLowerCase()

			placeholderText: credentialsGenerator.generateUsername()
			inputMethodHints: Qt.ImhPreferLowercase
			invalidHintText: qsTr("Please enter a valid username or leave the field empty for a random one.")
			valid: true

			// Validate the entered username and handle that if it is invalid.
			onTextChanged: {
				if (text === "")
					valid = true
				else
					valid = credentialsValidator.isUsernameValid(text)

				handleInvalidText()
			}
		}
	}
}
