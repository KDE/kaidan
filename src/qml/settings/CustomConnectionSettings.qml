// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

import "../elements"
import "../elements/fields"

RowLayout {
	property alias hostField: hostField
	property alias portField: portField

	// The type Item is used because the type Button does not work for buttons of type RoundButton.
	property Item confirmationButton

	Field {
		id: hostField
		labelText: qsTr("Hostname:")
		placeholderText: "xmpp.example.org"
		text: AccountManager.host
		inputMethodHints: Qt.ImhUrlCharactersOnly
		invalidHintText: qsTr("The hostname must not contain blank spaces")
		invalidHintMayBeShown: true

		onTextChanged: {
			valid = !text.match(/\s/);
			toggleHintForInvalidText()
		}

		// Focus the portField on confirmation.
		Keys.onPressed: {
			switch (event.key) {
			case Qt.Key_Return:
			case Qt.Key_Enter:
				portField.forceActiveFocus()
				event.accepted = true
			}
		}
	}

	ColumnLayout {
		// Position this field on top even if hostField.invalidHintText is shown.
		Layout.alignment: Qt.AlignCenter

		Controls.Label {
			text: qsTr("Port:")
		}

		Controls.SpinBox {
			id: portField
			editable: true
			from: AccountManager.portAutodetect
			to: 65535
			value: AccountManager.port
			Layout.minimumWidth: 80

			textFromValue: function(value, locale) {
				// Return an empty string if no custom port is set.
				if (value === AccountManager.portAutodetect)
					return ""

				// By returning the value without taking the locale into account, no digit grouping is applied.
				// Example: For a port number of "one thousand" the text "1000" instead of "1,000" is returned.
				return value
			}

			// Allow only an empty string (for no custom port) as input or input from 1 to 99999.
			// Without defining an own validator, zeros at the beginning and dots between the digits and at the end would be valid.
			validator: RegularExpressionValidator {
				regularExpression: /\b(\s|[1-9])[0-9]{4}/
			}

			// Simulate the pressing of the currently clickable confirmation button.
			Keys.onPressed: {
				switch (event.key) {
				case Qt.Key_Return:
				case Qt.Key_Enter:
					// Trigger that the text inside portField is set as its value.
					confirmationButton.forceActiveFocus()

					confirmationButton.clicked()
					event.accepted = true
				}
			}
		}
	}

	function forceActiveFocus() {
		hostField.forceActiveFocus()
	}
}
