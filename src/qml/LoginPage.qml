// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2018 Allan Nordhøy <epost@anotheragency.no>
// SPDX-FileCopyrightText: 2018 SohnyBohny <sohny.bean@streber24.de>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"
import "elements/fields"
import "settings"

Kirigami.Page {
	title: qsTr("Log in")

	ColumnLayout {
		anchors.fill: parent

		Kirigami.Heading {
			text: qsTr("Log in to your XMPP account")
			wrapMode: Text.WordWrap
			Layout.fillWidth: true
			horizontalAlignment: Qt.AlignHCenter
		}

		ColumnLayout {
			width: parent.width
			Layout.fillWidth: true

			// For desktop or tablet devices
			Layout.alignment: Qt.AlignCenter
			Layout.maximumWidth: largeButtonWidth

			// JID field
			JidField {
				id: jidField

				// Simulate the pressing of the loginButton.
				inputField {
					onAccepted: loginButton.clicked()
				}

				inputField.rightActions: [
					Kirigami.Action {
						icon.name: "preferences-system-symbolic"
						text: qsTr("Connection settings")
						onTriggered: {
							customConnectionSettings.visible = !customConnectionSettings.visible

							if (jidField.valid && customConnectionSettings.visible)
								customConnectionSettings.forceActiveFocus()
							else
								jidField.forceActiveFocus()
						}
					}
				]
			}

			CustomConnectionSettings {
				id: customConnectionSettings
				confirmationButton: loginButton
				visible: false
			}

			// password field
			PasswordField {
				id: passwordField

				// Simulate the pressing of the loginButton.
				inputField {
					onAccepted: loginButton.clicked()
				}
			}

			CenteredAdaptiveHighlightedButton {
				id: loginButton
				text: qsTr("Log in")

				state: Kaidan.connectionState !== Enums.StateDisconnected ? "connecting" : ""
				states: [
					State {
						name: "connecting"
						PropertyChanges {
							target: loginButton
							text: qsTr("Connecting…")
							enabled: false
						}
					}
				]

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
						AccountManager.jid = jidField.text
						AccountManager.password = passwordField.text
						AccountManager.host = customConnectionSettings.hostField.text
						AccountManager.port = customConnectionSettings.portField.value

						Kaidan.logIn()
					}
				}
			}
		}

		// placeholder
		Item {
			Layout.preferredHeight: Kirigami.Units.gridUnit * 3
		}
	}

	Component.onCompleted: {
		AccountManager.resetCustomConnectionSettings()
		jidField.forceActiveFocus()
	}

	/*
	 * Fills the JID field with "@" followed by a domain and moves the cursor to
	 * the beginning so that the username can be directly entered.
	 *
	 * \param domain domain being inserted into the JID field
	 */
	function prefillJidDomain(domain) {
		jidField.text = "@" + domain
		jidField.inputField.cursorPosition = 0
	}
}
