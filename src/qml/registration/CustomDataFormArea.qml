// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "../elements"

/**
 * This a form for custom data requested by a server for registration.
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
	}

	Kirigami.Separator {
		Layout.fillWidth: true
	}

	FormCard.AbstractFormDelegate {
		background: NonInteractiveFormDelegateBackground {}
		contentItem: ColumnLayout {
			DataForm {
				id: form
				displayTitle: false
				displayInstructions: false
				Layout.fillWidth: true
			}
		}
	}

	function forceActiveFocus() {
		form.nextItemInFocusChain().forceActiveFocus()
	}
}
