// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

GridLayout {
	id: root

	property string accountJid

	// Used for authenticating or distrusting keys via QR code scanning.
	property string chatJid
	readonly property bool forOwnDevices: accountJid === chatJid
	readonly property bool onlyForTrustDecisions: forOwnDevices || chatJid

	property alias scanner: scanner

	flow: parent.width > parent.height ? GridLayout.LeftToRight : GridLayout.TopToBottom
	rowSpacing: Kirigami.Units.largeSpacing * 2
	columnSpacing: rowSpacing
	width: flow === GridLayout.LeftToRight ? parent.width : Math.min(parent.width, parent.height / 2 - rowSpacing * 2)
	height: flow === GridLayout.TopToBottom ? parent.height : Math.min(parent.height, parent.width / 2 - columnSpacing * 2)

	QrCodeScanner {
		id: scanner

		property bool isAcceptingResult: true
		property bool isBusy: false

		Layout.fillWidth: parent.flow === GridLayout.TopToBottom
		Layout.fillHeight: parent.flow === GridLayout.LeftToRight
		Layout.preferredWidth: height
		Layout.preferredHeight: width
		Layout.alignment: Qt.AlignCenter

		// Use the data from the decoded QR code.
		filter.onScanningSucceeded: {
			if (isAcceptingResult) {
				isBusy = true
				let processTrust = true

				// Try to add a contact.
				if (!root.onlyForTrustDecisions) {
					switch (RosterModel.addContactByUri(root.accountJid, result)) {
					case RosterModel.AddingContact:
						showPassiveNotification(qsTr("Contact added - Continue with step 2"), Kirigami.Units.veryLongDuration * 4)
						break
					case RosterModel.ContactExists:
						processTrust = false
						break
					case RosterModel.InvalidUri:
						processTrust = false
						showPassiveNotification(qsTr("This QR code does not contain a contact"), Kirigami.Units.veryLongDuration * 4)
					}
				}

				// Try to authenticate or distrust keys.
				if (processTrust) {
					let expectedJid = ""

					if (root.onlyForTrustDecisions) {
						expectedJid = root.chatJid
					}

					switch (Kaidan.makeTrustDecisionsByUri(result, expectedJid)) {
					case Kaidan.MakingTrustDecisions:
						if (root.forOwnDevices) {
							showPassiveNotification(qsTr("Trust decisions made for other own device - Continue with step 2"), Kirigami.Units.veryLongDuration * 4)
						} else {
							showPassiveNotification(qsTr("Trust decisions made for contact - Continue with step 2"), Kirigami.Units.veryLongDuration * 4)
						}

						break
					case Kaidan.JidUnexpected:
						if (root.onlyForTrustDecisions) {
							if (root.forOwnDevices) {
								showPassiveNotification(qsTr("This QR code is not for your other device"), Kirigami.Units.veryLongDuration * 4)
							} else {
								showPassiveNotification(qsTr("This QR code is not for your contact"), Kirigami.Units.veryLongDuration * 4)
							}
						}
						break
					case Kaidan.InvalidUri:
						if (root.onlyForTrustDecisions) {
							showPassiveNotification(qsTr("This QR code is not for trust decisions"), Kirigami.Units.veryLongDuration * 4)
						}
					}
				}

				isBusy = false
				isAcceptingResult = false
				resetAcceptResultTimer.start()
			}
		}

		// timer to accept the result again after an invalid URI was scanned
		Timer {
			id: resetAcceptResultTimer
			interval: Kirigami.Units.veryLongDuration * 4
			onTriggered: scanner.isAcceptingResult = true
		}

		LoadingArea {
			description: root.onlyForTrustDecisions ? qsTr("Making trust decisions…") : qsTr("Adding contact…")
			anchors.centerIn: parent
			visible: scanner.isBusy
		}
	}

	Kirigami.Separator {
		Layout.fillWidth: parent.flow === GridLayout.TopToBottom
		Layout.fillHeight: !Layout.fillWidth
		Layout.topMargin: parent.flow === GridLayout.LeftToRight ? parent.height * 0.1 : 0
		Layout.bottomMargin: Layout.topMargin
		Layout.leftMargin: parent.flow === GridLayout.TopToBottom ? parent.width * 0.1 : 0
		Layout.rightMargin: Layout.leftMargin
		Layout.alignment: Qt.AlignCenter
	}

	QrCode {
		Layout.fillWidth: parent.flow === GridLayout.TopToBottom
		Layout.fillHeight: parent.flow === GridLayout.LeftToRight
		Layout.preferredWidth: height
		Layout.preferredHeight: width
		Layout.alignment: Qt.AlignCenter
	}
}
