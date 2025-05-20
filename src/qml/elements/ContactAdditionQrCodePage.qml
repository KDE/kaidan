// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

import im.kaidan.kaidan

/**
 * This page is used for adding contacts by scanning QR codes.
 */
ExplanationTogglePage {
	id: root

	property alias account: qrCodeScanningArea.account

	title: qsTr("Add contact")
	primaryButton.text: explanationArea.visible ? qsTr("Scan QR codes") : qsTr("Show explanation")
	primaryButton.onClicked: {
		if (Settings.contactAdditionQrCodePageExplanationVisible) {
			// Hide the explanation when this page is opened again in the future.
			Settings.contactAdditionQrCodePageExplanationVisible = false

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
	explanationArea.visible: Settings.contactAdditionQrCodePageExplanationVisible
	content: QrCodeScanningArea {
		id: qrCodeScanningArea
		anchors.centerIn: parent
		visible: !Settings.contactAdditionQrCodePageExplanationVisible
	}
	Component.onCompleted: {
		if (!Settings.contactAdditionQrCodePageExplanationVisible) {
			qrCodeScanningArea.scanner.cameraEnabled = true
		}
	}
}
