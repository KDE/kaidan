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

	property alias account: loginArea.account

	title: qsTr("Scan a QR Code")
	useMarginsForContent: false
	primaryButton.text: explanationArea.visible ? qsTr("Scan QR code") : qsTr("Show explanation")
	primaryButton.onClicked: {
		if (!scanner.cameraEnabled) {
			scanner.visible = true
			scanner.cameraEnabled = true
		}

		if (!scanner.acceptingResult) {
			scanner.acceptingResult = true
			loginArea.visible = false
			loginArea.reset()
		}
	}
	explanation: ColumnLayout {
		width: parent.width
		height: parent.height

		Item {
			visible: !loginArea.visible
			Layout.fillHeight: true
		}

		CenteredAdaptiveText {
			text: qsTr("Scan your old device's login QR code")
			scaleFactor: 1.5
			visible: !loginArea.visible
			Layout.topMargin: 10
		}

		CenteredAdaptiveText {
			text: qsTr("You can display the QR code in the account details of your old device via Show login QR code")
			scaleFactor: 1.2
			visible: !loginArea.visible
			Layout.topMargin: 10
		}

		Item {
			visible: !loginArea.visible
			Layout.fillHeight: true
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
		visible: false
		cornersRounded: false
		anchors.fill: parent
		zoomSlider.anchors.bottomMargin: Kirigami.Units.largeSpacing * 10
		zoomSlider.width: Math.min(largeButtonWidth, parent.width - Kirigami.Units.largeSpacing * 4)
		filter.onResultContentChanged: (result) => {
			if (result.hasText && acceptingResult) {
				// Try to log in by the data from the decoded QR code.
				switch (loginArea.account.logInWithUri(result.text)) {
				case Account.LoginWithUriResult.Connecting:
					acceptingResult = false
					break
				case Account.LoginWithUriResult.PasswordNeeded:
					root.primaryButton.clicked()
					acceptingResult = false
					loginArea.visible = true
					loginArea.initialize()
					break
				case Account.LoginWithUriResult.InvalidLoginUri:
					acceptingResult = false
					resetAcceptingResultTimer.start()
					showPassiveNotification(qsTr("This QR code is not a valid login QR code."), Kirigami.Units.veryLongDuration * 4)
					break
				}
			}
		}

		LoadingArea {
			description: qsTr("Connecting…")
			visible: loginArea.account && loginArea.account.connection.state === Enums.StateConnecting
			anchors.centerIn: parent
		}
	}
}
