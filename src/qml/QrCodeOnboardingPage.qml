// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "elements"

/**
 * This page is used for logging in by scanning a QR code which contains an XMPP login URI.
 */
ExplanationTogglePage {
	id: root
	title: qsTr("Scan a QR Code")
	useMarginsForContent: false
	primaryButton.text: explanationArea.visible ? qsTr("Scan QR code") : qsTr("Show explanation")
	primaryButton.onClicked: {
		if (!scanner.cameraEnabled) {
			scanner.cameraEnabled = true
		}

		if (!scanner.acceptResult) {
			scanner.acceptResult = true
			loginArea.visible = false
			loginArea.reset()
		}
	}
	explanation: ColumnLayout {
		width: parent.width
		height: parent.height

		CenteredAdaptiveText {
			text: qsTr("Scan your old device's QR code")
			scaleFactor: 1.5
			visible: !loginArea.visible
			Layout.topMargin: 10
		}

		Image {
			source: Utils.getResourcePath("images/onboarding/account-transfer.svg")
			sourceSize.height: root.height
			fillMode: Image.PreserveAspectFit
			visible: !loginArea.visible
			Layout.fillHeight: true
			Layout.fillWidth: true
		}

		LoginArea {
			id: loginArea
			visible: false
			Layout.alignment: Qt.AlignHCenter
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.fillWidth: true
		}
	}
	content: QrCodeScanner {
		id: scanner

		property bool acceptResult: true

		cornersRounded: false
		anchors.fill: parent
		zoomSlider.anchors.bottomMargin: Kirigami.Units.largeSpacing * 10
		zoomSlider.width: Math.min(largeButtonWidth, parent.width - Kirigami.Units.largeSpacing * 4)
		filter.onResultContentChanged: (result) => {
			if (result.hasText && acceptResult) {
				// Try to log in by the data from the decoded QR code.
				switch (Kaidan.logInByUri(result.text)) {
				case Enums.Connecting:
					acceptResult = false
					break
				case Enums.PasswordNeeded:
					root.primaryButton.clicked()
					acceptResult = false
					loginArea.visible = true
					loginArea.initialize()
					break
				case Enums.InvalidLoginUri:
					acceptResult = false
					resetAcceptResultTimer.start()
					showPassiveNotification(qsTr("This QR code is not a valid login QR code."), Kirigami.Units.veryLongDuration * 4)
					break
				}
			}
		}

		LoadingArea {
			description: qsTr("Connecting…")
			visible: Kaidan.connectionState === Enums.StateConnecting
			anchors.centerIn: parent
		}

		// timer to accept the result again after an invalid login URI was scanned
		Timer {
			id: resetAcceptResultTimer
			interval: Kirigami.Units.veryLongDuration * 4
			onTriggered: scanner.acceptResult = true
		}
	}
}
