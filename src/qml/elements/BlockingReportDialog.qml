// SPDX-FileCopyrightText: 2026 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * Dialog for reporting a message and blocking its sender.
 *
 * It is used for blocking a JID while referencing its messages.
 * Since it is opened via an explicit report action, reporting is mandatory.
 * Blocking without reporting is done via the contact details.
 */
Dialog {
	id: root

	property alias account: content.account
	property alias jid: content.jid
	property alias messageReferences: content.messageReferences

	title: qsTr("Block and report")
	onOpened: content.forceActiveFocus()

	ConfirmationArea {
		confirmationButton.text: qsTr("Block and report")
		confirmationButton.onClicked: {
			content.submit()
			root.close()
		}

		FormCard.FormTextDelegate {
			description: qsTr("Report the selected message and block its sender")
		}

		BlockingReportContent {
			id: content
			reportingMandatory: true
		}
	}
}
