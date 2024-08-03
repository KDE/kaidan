// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

/**
 * This page is used for authenticating encryption keys of group chat users by scanning QR codes or entering key IDs.
 */
UserKeyAuthenticationPage {
	id: root
	explanation: ExplanationArea {
		primaryExplanationText.text: qsTr("Step 1: Scan the <b>participant's</b> QR code")
		primaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-1.svg")
		secondaryExplanationText.text: qsTr("Step 2: Let the participant scan <b>your</b> QR code")
		secondaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-2.svg")
	}
	authenticatableKeysArea.header.title: qsTr("Unverified participant devices")
}
