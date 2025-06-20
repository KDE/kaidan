// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2018 Allan Nordhøy <epost@anotheragency.no>
// SPDX-FileCopyrightText: 2018 SohnyBohny <sohny.bean@streber24.de>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "fields"

FormCard.FormCard {
	id: root

	property Account account
	property alias title: header.title
	property alias qrCodeButton: qrCodeButton

	FormCard.FormHeader {
		id: header
		title: qsTr("Log in")
	}

	Kirigami.Separator {
		Layout.fillWidth: true
	}

	FormCard.FormButtonDelegate {
		id: qrCodeButton
		text: qsTr("Scan login QR code")
		visible: false
	}

	Kirigami.Separator {
		visible: qrCodeButton.visible
		Layout.fillWidth: true
	}

	FormCardCustomContentArea {
		contentItem: ColumnLayout {
			JidField {
				id: jidField
				text: root.account.settings.jid
				inputField.focus: true
				inputField.onAccepted: loginButton.clicked()
				inputField.rightActions: [
					Kirigami.Action {
						icon.name: "preferences-system-symbolic"
						text: qsTr("Connection settings")
						onTriggered: {
							customConnectionSettings.visible = !customConnectionSettings.visible

							if (jidField.valid && customConnectionSettings.visible) {
								customConnectionSettings.forceActiveFocus()
							} else {
								jidField.forceActiveFocus()
							}
						}
					}
				]
			}

			CustomConnectionSettings {
				id: customConnectionSettings
				account: root.account
				confirmationButton: loginButton
				visible: false
			}

			PasswordField {
				id: passwordField
				inputField.onAccepted: loginButton.clicked()
			}
		}
	}

	Kirigami.Separator {
		Layout.fillWidth: true
	}

	BusyIndicatorFormButton {
		id: loginButton
		idleText: qsTr("Log in")
		busyText: qsTr("Connecting…")
		busy: root.account && root.account.connection.state === Enums.StateConnecting
		background: HighlightedFormButtonBackground {}
		// Connect to the server and authenticate by the entered credentials if the JID is valid and a password entered.
		onClicked: {
			// If the JID is invalid, focus its field.
			if (!jidField.valid) {
				jidField.forceActiveFocus()
			// If the password is invalid, focus its field.
			// This also implies that if the JID field is focused and the password invalid, the password field will be focused instead of immediately trying to connect.
			} else if (!passwordField.valid) {
				passwordField.forceActiveFocus()
			} else {
				root.account.settings.jid = jidField.text
				root.account.settings.password = passwordField.text
				root.account.settings.host = customConnectionSettings.hostField.text
				root.account.settings.port = customConnectionSettings.portField.value

				root.account.connection.logIn()
			}
		}
	}

	function initialize() {
		if (jidField.valid) {
			passwordField.forceActiveFocus()
		} else if (jidField.text) {
			// This is used after a web registration when only the provider is known.
			// Prepend "@" to the server JID and move the cursor to the field's beginning.
			// That way, the username can be directly entered.
			// Ensure by checking whether "@" is already prepended that it is not prepended
			// multiple times by calling this function.
			if (jidField.text.charAt(0) !== "@") {
				jidField.text = "@" + jidField.text
			}

			jidField.inputField.forceActiveFocus()
			jidField.inputField.cursorPosition = 0
		} else {
			jidField.inputField.forceActiveFocus()
		}
	}

	function reset() {
		jidField.invalidHintMayBeShown = false
		jidField.toggleHintForInvalidText()

		passwordField.invalidHintMayBeShown = false
		passwordField.toggleHintForInvalidText()
	}

	function clearFields() {
		jidField.text = "";
		passwordField.text = "";
	}
}
