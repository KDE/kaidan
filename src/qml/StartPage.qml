// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
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
	id: root
	title: {
		if (!Kaidan.testAccountMigrationState(AccountMigrationManager.MigrationState.Idle)) {
			return qsTr("Account Migration")
		}

		return "Kaidan"
	}
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
	onBackRequested: {
		globalDrawer.enabled = true
		Kaidan.cancelAccountMigration()
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

				LoginArea {
					id: loginArea

					Connections {
						target: pageStack.layers

						function onCurrentItemChanged() {
							const currentItem = pageStack.layers.currentItem

							// Initialize loginArea on first opening (except after starting Kaidan)
							// and when going back from sub pages (i.e., layers above).
							// In the latter case, currentItem is confusingly not root but a
							// RowLayout.
							// As a workaround, currentItem is checked accordingly.
							if (currentItem === root || currentItem instanceof RowLayout) {
								loginArea.clearFields()
								loginArea.reset()
								loginArea.initialize()
							}
						}
					}

					Connections {
						target: pageStack

						function onCurrentItemChanged() {
							// Initialize loginArea when Kaidan is started.
							if (pageStack.currentItem === root) {
								loginArea.initialize()
							}
						}
					}

					Connections {
						target: applicationWindow()

						function onActiveFocusItemChanged() {
							// Ensure that loginArea is focused when this page is opened after an
							// account removal.
							// That workaround is needed because AccountDetailsSheet takes the focus
							// when it is closed once the account removal is completed.
							if (applicationWindow().activeFocusItem instanceof Controls.Overlay) {
								loginArea.initialize()
							}
						}
					}
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Scan login QR code")
					onClicked: pushLayer(qrCodeOnboardingPage)
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
					onClicked: pushLayer(automaticRegistrationPage)
				}

				MobileForm.FormButtonDelegate {
					text: qsTr("Create account manually")
					onClicked: pushLayer(manualRegistrationPage)
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
			if (Kaidan.connectionError !== ClientWorker.NoError) {
				passiveNotification(Utils.connectionErrorMessage(Kaidan.connectionError))
				loginArea.initialize()
			}
		}
	}
}
