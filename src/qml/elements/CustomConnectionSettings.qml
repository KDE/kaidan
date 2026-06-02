// SPDX-FileCopyrightText: 2021 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
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

	RowLayout {
		Field {
			id: hostField
			label: qsTr("Hostname")
			placeholderText: "xmpp.example.org"
			text: root.accountSettings.host
			inputMethodHints: Qt.ImhUrlCharactersOnly
			invalidHintText: qsTr("Enter a valid hostname or leave it empty")
			inputValidator.patterns: InputValidator.Pattern.Hostname | InputValidator.Pattern.Empty
			rightPadding: 0
			onEditingFinished: root.accountSettings.host = text
			onAccepted: portField.forceActiveFocus()
		}

		FormCard.FormSpinBoxDelegate {
			id: portField
			label: qsTr("Port")
			from: root.accountSettings.autoDetectPort
			to: 65535
			value: root.accountSettings.port
			textFromValue: function(value, locale) {
				return value === root.accountSettings.autoDetectPort ? "" : value.toString()
			}
			validator: InputValidator {
				patterns: InputValidator.Pattern.Port
			}
			leftPadding: 0
			Layout.alignment: Qt.AlignTop
			Layout.preferredWidth: Kirigami.Units.largeSpacing * 10
			Layout.fillWidth: false
			onValueChanged: root.accountSettings.port = value
			// TODO: Remove the following lines once Kirigami Addons > v1.12.1 is required by Kaidan.
			onActiveFocusChanged: {
				if (activeFocus) {
					portField.clicked()
				}
			}
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
