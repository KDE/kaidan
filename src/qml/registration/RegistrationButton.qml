// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This button is used for confirming the registration.
 */
CenteredAdaptiveHighlightedButton {
	property var loginFunction
	property var registrationFunction

	text: Kaidan.connectionError === ClientWorker.EmailConfirmationRequired ? qsTr("Login after confirmation") : qsTr("Register")
	onClicked: Kaidan.connectionError === ClientWorker.EmailConfirmationRequired ? loginFunction() : registrationFunction()
}
