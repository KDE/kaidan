// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Yuri Chornoivan <yurchor@ukr.net>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls

import im.kaidan.kaidan 1.0

import "../elements/fields"

/**
 * This view is used for entering a password.
 */
FieldView {
	descriptionText: qsTr("You can provide a password for your account. If you don't, a password will be generated that you can find later in the app.")
	imageSource: "password"

	property string text: field.text.length > 0 ? field.text : field.generatedPassword

	property alias valid: field.valid

	ColumnLayout {
		parent: contentArea

		RegistrationPasswordField {
			id: field

			// Validate the entered password and handle that if it is invalid.
			onTextChanged: {
				if (text === "")
					valid = true
				else
					valid = credentialsValidator.isPasswordValid(text)

				handleInvalidText()
			}
		}
	}
}
