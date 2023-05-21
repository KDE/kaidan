// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

import "../elements"

/**
 * This view is used for presenting the result of the registration process and confirming the registration.
 */
View {
	descriptionText: jid ? qsTr("Your chat address is used to chat with you, like your email address is used to exchange email messages.\n\nYour chat address will be:") : ""
	imageSource: "result"

	property alias registrationButton: registrationButton

	property string jid: username && provider ? username + "@" + provider : ""

	ColumnLayout {
		parent: contentArea
		spacing: 40

		CenteredAdaptiveText {
			text: jid
			scaleFactor: 1.5
		}

		RegistrationButton {
			id: registrationButton
		}
	}
}
