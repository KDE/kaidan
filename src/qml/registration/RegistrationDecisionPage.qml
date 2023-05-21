// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This page is used for deciding between the automatic or manual registration.
 */
BinaryDecisionPage {
	title: qsTr("Register")

	topImageSource: Utils.getResourcePath("images/onboarding/automatic-registration.svg")
	bottomImageSource: Utils.getResourcePath("images/onboarding/manual-registration.svg")

	topAction: Kirigami.Action {
		text: qsTr("Generate an account automatically")
		onTriggered: pageStack.layers.push(automaticRegistrationPage)
	}

	bottomAction: Kirigami.Action {
		text: qsTr("Create an account manually")
		onTriggered: pageStack.layers.push(manualRegistrationPage)
	}
}
