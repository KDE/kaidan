// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"
import "../elements/fields"

ColumnLayout {
	id: root

	property AccountSettings accountSettings
	property bool changesPending: _previousHost !== hostField.text || _previousPort !== portField.value || _previousPlainAuthAllowed !== plainAuthField.checked
	property string _previousHost
	property int _previousPort
	property bool _previousPlainAuthAllowed

	// The type Item is used because the type Button does not work for buttons of type RoundButton.
	property Item confirmationButton

	spacing: 0
	Component.onCompleted: resetChangesPending()

	FormCard.AbstractFormDelegate {
		background: null
		contentItem: RowLayout {
			Field {
				id: hostField
				labelText: qsTr("Hostname:")
				placeholderText: "xmpp.example.org"
				text: root.accountSettings.host
				inputMethodHints: Qt.ImhUrlCharactersOnly
				invalidHintText: qsTr("The hostname must not contain blank spaces")
				invalidHintMayBeShown: true
				valid: !text.match(/\s/)
				inputField.onEditingFinished: root.accountSettings.host = text
				// Focus the portField on confirmation.
				Keys.onPressed: event => {
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
					from: root.accountSettings.autoDetectPort
					to: 65535
					value: root.accountSettings.port
					live: true
					Layout.minimumWidth: 80
					textFromValue: function(value, locale) {
						// Return an empty string if no custom port is set.
						if (value === root.accountSettings.autoDetectPort) {
							return ""
						}

						// By returning the value without taking the locale into account, no digit grouping is applied.
						// Example: For a port number of "one thousand" the text "1000" instead of "1,000" is returned.
						return value
					}
					// Allow only an empty string (for no custom port) as input or input from 1 to 99999.
					// Without defining an own validator, zeros at the beginning and dots between the digits and at the end would be valid.
					validator: RegularExpressionValidator {
						regularExpression: /\b(\s|[1-9])[0-9]{4}/
					}
					onValueChanged: root.accountSettings.port = value
					// Simulate the pressing of the currently clickable confirmation button.
					Keys.onPressed: event => {
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
		}
	}

	FormCard.FormSwitchDelegate {
		id: plainAuthField
		text: qsTr("Plain authentication (SASL PLAIN)")
		description: qsTr("Required if your provider uses LDAP which is often the case in organizations where you can log in with a single account")
		checked: root.accountSettings.plainAuthAllowed
		onClicked: root.accountSettings.plainAuthAllowed = checked
	}

	function forceActiveFocus() {
		hostField.forceActiveFocus()
	}

	function resetChangesPending() {
		_previousHost = hostField.text
		_previousPort = portField.value
		_previousPlainAuthAllowed = plainAuthField.checked
	}

	function undoPendingChanges() {
		accountSettings.host = _previousHost
		accountSettings.port = _previousPort
		accountSettings.plainAuthAllowed = _previousPlainAuthAllowed
	}
}
