/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page is used for scanning QR codes and displaying an own QR code which
 * can be scanned.
 */
ExplanationTogglePage {
	id: root

	// Used for authenticating or distrusting keys via QR code scanning.
	property bool isOnlyForTrustDecisions: isForOwnDevices || contactJid
	property bool isForOwnDevices: false
	property string contactJid

	title: qsTr("Scan QR codes")
	explanationArea.visible: Kaidan.settings.qrCodePageExplanationVisible
	explanationToggleButton.text: explanationToggleButton.checked ? qsTr("Show explanation") : qsTr("Scan QR codes")
	explanationToggleButton.checked: !Kaidan.settings.qrCodePageExplanationVisible

	explanationToggleButton.onClicked: {
		if (Kaidan.settings.qrCodePageExplanationVisible) {
			// Hide the explanation when this page is opened again in the future.
			Kaidan.settings.qrCodePageExplanationVisible = false

			if (!scanner.cameraEnabled) {
				scanner.camera.start()
				scanner.cameraEnabled = true
			}
		}
	}

	secondaryButton.visible: false

	explanation: GridLayout {
		flow: applicationWindow().wideScreen ? GridLayout.LeftToRight : GridLayout.TopToBottom

		ColumnLayout {
			CenteredAdaptiveText {
				text: {
					if (root.isForOwnDevices) {
						return qsTr("Step 1: Scan your <b>other device's</b> QR code")
					}
					return qsTr("Step 1: Scan your <b>contact's</b> QR code")
				}
				scaleFactor: 1.5
			}

			Image {
				source: Utils.getResourcePath(root.isForOwnDevices ? "images/qr-code-scan-own-1.svg" : "images/qr-code-scan-1.svg")
				sourceSize: Qt.size(860, 860)
				fillMode: Image.PreserveAspectFit
				mipmap: true
				Layout.fillHeight: true
				Layout.fillWidth: true
			}
		}

		Kirigami.Separator {
			Layout.fillWidth: applicationWindow().wideScreen ? false : true
			Layout.fillHeight: !Layout.fillWidth
			Layout.topMargin: applicationWindow().wideScreen ? parent.height * 0.1 : parent.height * 0.01
			Layout.bottomMargin: Layout.topMargin
			Layout.leftMargin: applicationWindow().wideScreen ? parent.width * 0.01 : parent.width * 0.1
			Layout.rightMargin: Layout.leftMargin
		}

		ColumnLayout {
			CenteredAdaptiveText {
				text: {
					if (root.isForOwnDevices) {
						return qsTr("Step 2: Scan with your other device <b>this device's</b> QR code")
					}
					return qsTr("Step 2: Let your contact scan <b>your</b> QR code")
				}
				scaleFactor: 1.5
			}

			Image {
				source: Utils.getResourcePath(root.isForOwnDevices ? "images/qr-code-scan-own-2.svg" : "images/qr-code-scan-2.svg")
				sourceSize: Qt.size(860, 860)
				fillMode: Image.PreserveAspectFit
				Layout.fillHeight: true
				Layout.fillWidth: true
			}
		}
	}

	content: GridLayout {
		anchors.centerIn: parent
		flow: applicationWindow().wideScreen ? GridLayout.LeftToRight : GridLayout.TopToBottom
		visible: !Kaidan.settings.qrCodePageExplanationVisible
		width: applicationWindow().wideScreen ? parent.width : Math.min(largeButtonWidth, parent.width, parent.height * 0.48)
		height: applicationWindow().wideScreen ? Math.min(parent.height, parent.width * 0.48) : parent.height

		QrCodeScanner {
			id: scanner
			Layout.preferredWidth: applicationWindow().wideScreen ? parent.width * 0.48 : parent.width
			Layout.preferredHeight: applicationWindow().wideScreen ? parent.height : Layout.preferredWidth

			// Use the data from the decoded QR code.
			filter.onScanningSucceeded: {
				if (isAcceptingResult) {
					isBusy = true
					var processTrust = true

					// Try to add a contact.
					if (!root.isOnlyForTrustDecisions) {
						switch (RosterModel.addContactByUri(result)) {
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
						var expectedJid = ""
						if (root.isOnlyForTrustDecisions) {
							expectedJid = root.isForOwnDevices ? AccountManager.jid : root.contactJid
						}
						switch (Kaidan.makeTrustDecisionsByUri(result, expectedJid)) {
						case Kaidan.MakingTrustDecisions:
							if (root.isForOwnDevices) {
								showPassiveNotification(qsTr("Trust decisions made for other own device - Continue with step 2"), Kirigami.Units.veryLongDuration * 4)
							} else {
								showPassiveNotification(qsTr("Trust decisions made for contact - Continue with step 2"), Kirigami.Units.veryLongDuration * 4)
							}

							break
						case Kaidan.JidUnexpected:
							if (root.isOnlyForTrustDecisions) {
								if (root.isForOwnDevices) {
									showPassiveNotification(qsTr("This QR code is not for your other device"), Kirigami.Units.veryLongDuration * 4)
								} else {
									showPassiveNotification(qsTr("This QR code is not for your contact"), Kirigami.Units.veryLongDuration * 4)
								}
							}
							break
						case Kaidan.InvalidUri:
							if (root.isOnlyForTrustDecisions) {
								showPassiveNotification(qsTr("This QR code is not for trust decisions"), Kirigami.Units.veryLongDuration * 4)
							}
						}
					}

					isBusy = false
					isAcceptingResult = false
					resetAcceptResultTimer.start()
				}
			}

			property bool isAcceptingResult: true
			property bool isBusy: false

			// timer to accept the result again after an invalid URI was scanned
			Timer {
				id: resetAcceptResultTimer
				interval: Kirigami.Units.veryLongDuration * 4
				onTriggered: scanner.isAcceptingResult = true
			}

			Item {
				anchors.centerIn: parent

				// background of loadingArea
				Rectangle {
					anchors.fill: loadingArea
					anchors.margins: -8
					radius: roundedCornersRadius
					color: Kirigami.Theme.backgroundColor
					opacity: 0.9
					visible: loadingArea.visible
				}

				ColumnLayout {
					id: loadingArea
					anchors.centerIn: parent
					visible: scanner.isBusy

					Controls.BusyIndicator {
						Layout.alignment: Qt.AlignHCenter
					}

					Controls.Label {
						text: "<i>" + root.isOnlyForTrustDecisions ? qsTr("Making trust decisions…") : qsTr("Adding contact…") + "</i>"
						color: Kirigami.Theme.textColor
					}
				}
			}
		}

		Kirigami.Separator {
			Layout.fillWidth: !applicationWindow().wideScreen
			Layout.fillHeight: !Layout.fillWidth
			Layout.topMargin: applicationWindow().wideScreen ? parent.height * 0.1 : parent.height * 0.01
			Layout.bottomMargin: Layout.topMargin
			Layout.leftMargin: applicationWindow().wideScreen ? parent.width * 0.01 : parent.width * 0.1
			Layout.rightMargin: Layout.leftMargin
		}

		QrCode {
			Layout.preferredWidth: applicationWindow().wideScreen ? parent.width * 0.48 : parent.width
			Layout.preferredHeight: applicationWindow().wideScreen ? parent.height : Layout.preferredWidth
		}
	}

	Component.onCompleted: {
		if (!Kaidan.settings.qrCodePageExplanationVisible) {
			scanner.camera.start()
			scanner.cameraEnabled = true
		}
	}
}
