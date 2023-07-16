// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

import im.kaidan.kaidan 1.0

/**
 * This is used for logging in to the server with stored credentials if not connected.
 */
ChatHintArea {
	id: root

	ConfirmationDismissArea {
		id: actionArea
		cancelButton.onClicked: close()
		confirmationButton.text: qsTr("Connect")
		confirmationButton.onClicked: logIn()
		loadingArea.background.visible: false
		loadingArea.description: qsTr("Connecting…")

		CenteredAdaptiveText {
			text: qsTr("You are not connected. Messages are sent once connected.")
		}
	}

	Connections {
		target: Kaidan

		function onConnectionStateChanged() {
			if (Kaidan.connectionState === Enums.StateConnected) {
				root.close()
			} else if (Kaidan.connectionState === Enums.StateDisconnected) {
				actionArea.hideLoadingView()
				globalDrawer.open()
			}
		}

		function onConnectionErrorChanged() {
			if (Kaidan.connectionError) {
				globalDrawer.open()
			} else {
				actionArea.hideLoadingView()
			}
		}
	}

	function logIn() {
		actionArea.showLoadingView()
		Kaidan.logIn()
	}
}
