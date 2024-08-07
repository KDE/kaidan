// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page is used for logging in by scanning a QR code which contains an XMPP login URI.
 */
ExplanationTogglePage {
	id: root
	title: qsTr("Scan QR code")
	useMarginsForContent: false
	primaryButton.text: explanationArea.visible ? qsTr("Scan QR code") : qsTr("Show explanation")
	primaryButton.onClicked: {
		if (!scanner.cameraEnabled) {
			scanner.camera.start()
			scanner.cameraEnabled = true
		}
	}
	explanation: ColumnLayout {
		width: parent.width
		height: parent.height

		CenteredAdaptiveText {
			text: qsTr("Scan your old device's QR code")
			Layout.topMargin: 10
			scaleFactor: 1.5
		}

		Image {
			source: Utils.getResourcePath("images/onboarding/account-transfer.svg")
			sourceSize.height: root.height
			fillMode: Image.PreserveAspectFit
			Layout.fillHeight: true
			Layout.fillWidth: true
		}
	}
	content: QrCodeScanner {
		id: scanner

		property bool acceptResult: true

		cornersRounded: false
		anchors.fill: parent
		zoomSliderArea.anchors.bottomMargin: Kirigami.Units.largeSpacing * 9
		zoomSliderArea.width: Math.min(largeButtonWidth, parent.width - Kirigami.Units.largeSpacing * 4)
		filter.onScanningSucceeded: {
			if (acceptResult) {
				// Try to log in by the data from the decoded QR code.
				switch (Kaidan.logInByUri(result)) {
				case Enums.Connecting:
					acceptResult = false
					break
				case Enums.PasswordNeeded:
					acceptResult = false
					popLayersAboveLowest()
					break
				case Enums.InvalidLoginUri:
					acceptResult = false
					resetAcceptResultTimer.start()
					showPassiveNotification(qsTr("This QR code is not a valid login QR code."), Kirigami.Units.veryLongDuration * 4)
				}
			}
		}

		LoadingArea {
			anchors.centerIn: parent
			description: qsTr("Connecting…")
			visible: Kaidan.connectionState === Enums.StateConnecting
		}

		// timer to accept the result again after an invalid login URI was scanned
		Timer {
			id: resetAcceptResultTimer
			interval: Kirigami.Units.veryLongDuration * 4
			onTriggered: scanner.acceptResult = true
		}
	}

	onBackRequested: function (event) {
		if (!Kaidan.testAccountMigrationState(AccountMigrationManager.MigrationState.Idle)) {
			event.accepted = true
			Kaidan.openStartPageRequested()
		}
	}
}
