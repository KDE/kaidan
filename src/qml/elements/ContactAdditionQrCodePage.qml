// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14

import im.kaidan.kaidan 1.0

/**
 * This page is used for adding contacts by scanning QR codes.
 */
ExplanationTogglePage {
	id: root
	title: qsTr("Add contact")
	primaryButton.text: explanationArea.visible ? qsTr("Scan QR codes") : qsTr("Show explanation")
	primaryButton.onClicked: {
		if (Kaidan.settings.contactAdditionQrCodePageExplanationVisible) {
			// Hide the explanation when this page is opened again in the future.
			Kaidan.settings.contactAdditionQrCodePageExplanationVisible = false

			if (!qrCodeScanningArea.scanner.cameraEnabled) {
				qrCodeScanningArea.scanner.cameraEnabled = true
			}
		}
	}
	secondaryButton.visible: false
	explanation: ExplanationArea {
		primaryExplanationText.text: qsTr("Step 1: Scan your <b>contact's</b> QR code")
		primaryExplanationImage.source:  Utils.getResourcePath("images/qr-code-scan-1.svg")
		secondaryExplanationText.text: qsTr("Step 2: Let your contact scan <b>your</b> QR code")
		secondaryExplanationImage.source: Utils.getResourcePath("images/qr-code-scan-2.svg")
	}
	explanationArea.visible: Kaidan.settings.contactAdditionQrCodePageExplanationVisible
	content: QrCodeScanningArea {
		id: qrCodeScanningArea
		accountJid: AccountManager.jid
		anchors.centerIn: parent
		visible: !Kaidan.settings.contactAdditionQrCodePageExplanationVisible
	}
	Component.onCompleted: {
		if (!Kaidan.settings.contactAdditionQrCodePageExplanationVisible) {
			qrCodeScanningArea.scanner.cameraEnabled = true
		}
	}
}
