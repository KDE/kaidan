// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan 1.0

/**
 * This page is used for authenticating encryption keys of contacs by scanning QR codes or entering key IDs.
 */
UserKeyAuthenticationPage {
	id: root
	explanation: ExplanationArea {
		primaryExplanationText.text: qsTr("Step 1: Scan your <b>contact's</b> QR code")
		primaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-1.svg")
		secondaryExplanationText.text: qsTr("Step 2: Let your contact scan <b>your</b> QR code")
		secondaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-2.svg")
	}
	authenticatableKeysArea.header.title: qsTr("Unverified contact devices")
}
