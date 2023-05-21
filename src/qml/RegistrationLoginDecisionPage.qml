// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page is used for deciding between registration or login.
 */
BinaryDecisionPage {
	title: qsTr("Set up")

	topImageSource: Utils.getResourcePath("images/onboarding/registration.svg")
	bottomImageSource: Utils.getResourcePath("images/onboarding/login.svg")

	topAction: Kirigami.Action {
		text: qsTr("Register a new account")
		onTriggered: pageStack.layers.push(registrationDecisionPage)
	}

	bottomAction: Kirigami.Action {
		text: qsTr("Use an existing account")
		onTriggered: pageStack.layers.push(loginPage)
	}
}
