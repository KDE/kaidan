// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"

/**
 * This is an area containing a button used for confirming a registration.
 */
FormCard.FormCard {
	id: root

	property Account account
	property var registrationFunction
	property var loginFunction
	property alias busy: button.busy

	BusyIndicatorFormButton {
		id: button
		idleText: root.account.connection.error === ClientController.EmailConfirmationRequired ? qsTr("Log in after email confirmation") : qsTr("Register")
		busyText: root.account.connection.error === ClientController.EmailConfirmationRequired ? qsTr("Logging in…") : qsTr("Registering…")
		background: HighlightedFormButtonBackground {}
		onClicked: {
			if (root.account.connection.error === ClientController.EmailConfirmationRequired) {
				loginFunction()
			} else {
				registrationFunction()
			}

			busy = true
		}

		Connections {
			target: root.account.connection

			function onErrorChanged(error) {
				button.busy = false
			}
		}

		Connections {
			target: root.account.registrationController

			function onRegistrationFailed(error, errorMessage) {
				button.busy = false
			}
		}
	}
}
