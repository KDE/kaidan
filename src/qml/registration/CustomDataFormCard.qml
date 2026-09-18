// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"

/**
 * This is a card for custom form data requested by a server for registration.
 */
FormCard.FormCard {
	id: root

	property alias model: form.model
	property alias lastTextFieldAcceptedFunction: form.lastTextFieldAcceptedFunction

	FormCard.FormHeader {
		title: qsTr("Enter additional information")
	}

	FormCard.FormTextDelegate {
		text: qsTr("The provider has requested more information")
		description: qsTr("Not everything may be required")
		background: NonInteractiveFormDelegateBackground {}
	}

	DataForm {
		id: form
	}

	function forceActiveFocus() {
		form.nextItemInFocusChain().forceActiveFocus()
	}
}
