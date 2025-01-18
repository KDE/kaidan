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
	property var registrationFunction
	property var loginFunction
	property alias busy: button.busy

	BusyIndicatorFormButton {
		id: button
		idleText: Kaidan.connectionError === ClientWorker.EmailConfirmationRequired ? qsTr("Log in after email confirmation") : qsTr("Register")
		busyText: Kaidan.connectionError === ClientWorker.EmailConfirmationRequired ? qsTr("Logging in…") : qsTr("Registering…")
		background: HighlightedFormButtonBackground {}
		onClicked: {
			if (Kaidan.connectionError === ClientWorker.EmailConfirmationRequired) {
				loginFunction()
			} else {
				registrationFunction()
			}

			busy = true
		}

		Connections {
			target: Kaidan

			function onRegistrationFailed(error, errorMessage) {
				button.busy = false
			}

			function onConnectionErrorChanged(error) {
				button.busy = false
			}
		}
	}
}
