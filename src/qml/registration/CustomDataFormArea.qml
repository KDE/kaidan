// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This a form for custom data requested by a server for registration.
 */
MobileForm.FormCard {
	id: root

	property alias model: form.model
	property alias lastTextFieldAcceptedFunction: form.lastTextFieldAcceptedFunction

	contentItem: ColumnLayout {
		spacing: 0

		MobileForm.FormCardHeader {
			title: qsTr("Enter additional information")
		}

		MobileForm.FormTextDelegate {
			text: qsTr("The provider has requested more information")
			description: qsTr("Not everything may be required")
		}

		MobileForm.FormCard {
			Layout.fillWidth: true
			Kirigami.Theme.colorSet: Kirigami.Theme.Window
			contentItem: MobileForm.AbstractFormDelegate {
				background: null
				contentItem: ColumnLayout {
					DataForm {
						id: form
						displayTitle: false
						displayInstructions: false
						Layout.fillWidth: true
					}
				}
			}
		}
	}

	function forceActiveFocus() {
		form.nextItemInFocusChain().forceActiveFocus()
	}
}
