// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "elements"

/**
 * This page is the first page.
 *
 * It is displayed if no account is available.
 */
Kirigami.ScrollablePage {
	title: "Kaidan"
	background: Rectangle {
		color: secondaryBackgroundColor

		Image {
			source: Utils.getResourcePath("images/chat-page-background.svg")
			anchors.fill: parent
			fillMode: Image.Tile
			horizontalAlignment: Image.AlignLeft
			verticalAlignment: Image.AlignTop
		}
	}

	ColumnLayout {
		Item {
			Layout.fillHeight: true
		}

		Image {
			source: Utils.getResourcePath("images/kaidan.svg")
			fillMode: Image.PreserveAspectFit
			mipmap: true
			sourceSize.width: width
			sourceSize.height: height
			Layout.alignment: Qt.AlignHCenter
		}

		CenteredAdaptiveText {
			text: "Kaidan"
			scaleFactor: 4
		}

		CenteredAdaptiveText {
			text: qsTr("Enjoy free communication on every device!")
			Layout.bottomMargin: Kirigami.Units.largeSpacing * 4
			scaleFactor: 1.5
		}

		MobileForm.FormCard {
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Login")
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Enter your credentials")
					checkable: true
					onClicked: loginArea.visible = !loginArea.visible
				}

				LoginArea {
					id: loginArea
					visible: false
					onVisibleChanged: {
						if (visible) {
							initialize()
						} else {
							reset()
						}
					}

					Connections {
						target: pageStack.layers

						function onCurrentItemChanged() {
							if (AccountManager.jid) {
								loginArea.visible = true
							}
						}
					}
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Scan login QR code of old device")
					onClicked: openPage(qrCodeOnboardingPage)
				}
			}
		}

		Kirigami.Heading {
			text: qsTr("or")
			horizontalAlignment: Text.AlignHCenter
			Layout.fillWidth: true
		}

		MobileForm.FormCard {
			Layout.alignment: Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Register")
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Generate account automatically")
					onClicked: openPage(automaticRegistrationPage)
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Create account manually")
					onClicked: openPage(manualRegistrationPage)
				}
			}
		}

		Item {
			Layout.fillHeight: true
		}
	}

	Connections {
		target: Kaidan

		function onConnectionErrorChanged() {
			if (Kaidan.connectionError !== ClientWorker.NoError)
				passiveNotification(Utils.connectionErrorMessage(Kaidan.connectionError))
		}
	}
}
