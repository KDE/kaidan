// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Controls 2.15 as Controls

import im.kaidan.kaidan 1.0

/**
 * This view is used for entering the values into the custom fields of the received registration form during the automatic registration.
 */
CustomFormView {
	property alias registrationButton: registrationButton

	RegistrationButton {
		id: registrationButton
		loginFunction: () => {
			addLoadingArea()
			changeNickname()
			Kaidan.logIn()
		}
		registrationFunction: sendRegistrationFormAndShowLoadingArea
	}

	// This is used for automatically focusing the first field of the form.
	Controls.StackView.onStatusChanged: {
		if (Controls.StackView.status === Controls.StackView.Active)
			forceActiveFocus()
	}
}
