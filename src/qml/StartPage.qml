// SPDX-FileCopyrightText: 2020 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "elements"
import "registration"

/**
 * This page is the first page.
 *
 * It is displayed if no account is available.
 */
ImageBackgroundPage {
	id: root
	title: {
		if (Kaidan.accountMigrationController.migrationState !== AccountMigrationController.MigrationState.Idle) {
			return qsTr("Account Migration")
		}

		return "Kaidan"
	}
	onBackRequested: {
		globalDrawer.enabled = true
		Kaidan.accountMigrationController.cancelMigration()
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

		LoginArea {
			id: loginArea
			qrCodeButton {
				visible: true
				onClicked: pushLayer(qrCodeOnboardingPage)
			}
			Layout.alignment: Qt.AlignHCenter
			Layout.fillWidth: true

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
				target: applicationWindow()

				function onActiveFocusItemChanged() {
					// Ensure that loginArea is focused when this page is opened after an
					// account removal.
					// That workaround is needed because AccountDetailsDialog takes the
					// focus when it is closed once the account removal is completed.
					if (applicationWindow().activeFocusItem instanceof Controls.Overlay) {
						loginArea.initialize()
					}
				}
			}
		}

		FormCard.FormCard {
			Layout.alignment: Qt.AlignHCenter
			Layout.fillWidth: true

			FormCard.FormHeader {
				title: qsTr("Register")
			}

			FormCard.FormButtonDelegate {
				text: qsTr("Generate account automatically")
				onClicked: pushLayer(automaticRegistrationPage)
			}

			FormCard.FormButtonDelegate {
				text: qsTr("Create account manually")
				onClicked: pushLayer(providerPage)
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

	Component {
		id: automaticRegistrationPage

		AutomaticRegistrationPage {}
	}

	Component {
		id: providerPage

		ProviderPage {}
	}
}
