// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import "../elements"

/**
 * This button is used for confirming the registration.
 */
CenteredAdaptiveHighlightedButton {
	text: qsTr("Register")
	onClicked: sendRegistrationFormAndShowLoadingView()
}
