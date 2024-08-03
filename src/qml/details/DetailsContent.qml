// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2024 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import ".."
import "../elements"

Controls.Control {
	id: root

	default property alias __data: mainArea.data
	property Kirigami.OverlaySheet sheet
	required property string jid
	property alias topArea: topArea.data
	property alias automaticMediaDownloadsDelegate: automaticMediaDownloadsDelegate
	property alias mediaOverview: mediaOverview
	property alias mediaOverviewExpansionButton: mediaOverviewExpansionButton
	property alias vCardArea: vCardArea
	property alias vCardContentArea: vCardContentArea.data
	property alias vCardRepeater: vCardRepeater
	property alias rosterGoupListView: rosterGoupListView
	property alias sharingArea: sharingArea
	property alias qrCodeExpansionButton: qrCodeExpansionButton
	property alias qrCode: qrCodeArea.contentItem
	property alias qrCodeButton: qrCodeButton
	property alias uriButton: uriButton
	property alias invitationButton: invitationButton
	required property ColumnLayout encryptionArea

	topPadding: Kirigami.Settings.isMobile ? Kirigami.Units.largeSpacing : Kirigami.Units.largeSpacing * 3
	bottomPadding: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.largeSpacing * 3
	leftPadding: bottomPadding
	rightPadding: leftPadding
	background: Rectangle {
		color: secondaryBackgroundColor
	}

	contentItem: ColumnLayout {
		id: mainArea
		spacing: Kirigami.Units.largeSpacing

		ColumnLayout {
			id: topArea
		}

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Media")
				}

				MobileForm.FormComboBoxDelegate {
					id: automaticMediaDownloadsDelegate
					text: qsTr("Automatic Downloads")
					description: qsTr("Download media automatically")

					// "FormComboBoxDelegate.indexOfValue()" seems to not work with an array-based
					// model.
					// Thus, an own function is used.
					function indexOf(value) {
						if (Array.isArray(model)) {
							return model.findIndex((entry) => entry[valueRole] === value)
						}

						return indexOfValue(value)
					}

					Component.onCompleted: {
						// "Kirigami.OverlaySheet" uses a z-index of 101.
						// In order to see the popup, it needs to have that z-index as well.
						if (root.sheet) {
							let comboBox = contentItem.children[2];

							if (comboBox instanceof Controls.ComboBox) {
								comboBox.popup.z = 101
							}
						}
                    }
				}

				ColumnLayout {
					visible: mediaOverviewExpansionButton.visible && mediaOverviewExpansionButton.checked

					MobileForm.FormSectionText {
						text: qsTr("You can share media up to %1.").arg(Kaidan.serverFeaturesCache.httpUploadLimitString)
					}

					ColumnLayout {
						visible: mediaOverview.totalFilesCount
						spacing: 0

						Kirigami.Separator {
							Layout.fillWidth: true
						}

						MediaOverview {
							id: mediaOverview
							Layout.fillWidth: true
						}
					}
				}

				FormExpansionButton {
					id: mediaOverviewExpansionButton
					onCheckedChanged: {
						if (checked) {
							mediaOverview.selectionMode = false

							// Display the content of the first tab only on initial loading.
							// Afterwards, display the content of the last active tab.
							if (mediaOverview.tabBarCurrentIndex === -1) {
								mediaOverview.tabBarCurrentIndex = 0
							}

							mediaOverview.loadDownloadedFiles()
						}
					}
				}
			}
		}

		MobileForm.FormCard {
			id: vCardArea
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				id: vCardContentArea
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Profile")
				}

				Repeater {
					id: vCardRepeater
					Layout.fillHeight: true
				}
			}
		}

		MobileForm.FormCard {
			Layout.fillWidth: true
			contentItem: root.encryptionArea
		}

		MobileForm.FormCard {
			// Hide this if there are no items and no header.
			visible: rosterGoupListView.count || rosterGoupListView.headerItem
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Labels")
				}

				ListView {
					id: rosterGoupListView
					model: RosterModel.groups
					visible: rosterGroupExpansionButton.checked
					implicitHeight: contentHeight
					Layout.fillWidth: true
				}

				FormExpansionButton {
					id: rosterGroupExpansionButton
				}
			}
		}

		MobileForm.FormCard {
			id: sharingArea
			Layout.fillWidth: true

			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Sharing")
				}

				MobileForm.FormButtonDelegate {
					id: qrCodeExpansionButton
					text: qsTr("Show QR code")
					icon.name: "view-barcode-qr"
					// TODO: If possible, scroll down to show whole QR code
					onClicked: qrCodeArea.visible = !qrCodeArea.visible
				}

				MobileForm.AbstractFormDelegate {
					id: qrCodeArea
					visible: false
					background: Rectangle {
						color: secondaryBackgroundColor
					}
					Layout.preferredWidth: parent.width
					Layout.preferredHeight: Layout.preferredWidth
				}

				MobileForm.FormButtonDelegate {
					id: qrCodeButton
					text: qsTr("Copy QR code")
					icon.name: "send-to-symbolic"
					onClicked: passiveNotification(qsTr("QR code copied to clipboard"))
				}

				MobileForm.FormButtonDelegate {
					id: uriButton
					text: qsTr("Copy chat address")
					icon.name: "send-to-symbolic"
				}

				MobileForm.FormButtonDelegate {
					id: invitationButton
					text: qsTr("Copy invitation link")
					icon.name: "mail-message-new-symbolic"
					onClicked: passiveNotification(qsTr("Invitation link copied to clipboard"))
				}
			}
		}

		MobileForm.FormCard {
			visible: deviceRepeater.count
			Layout.fillWidth: true
			contentItem: ColumnLayout {
				spacing: 0

				MobileForm.FormCardHeader {
					title: qsTr("Connected Devices")
				}

				Repeater {
					id: deviceRepeater
					Layout.fillHeight: true
					model: UserDevicesModel {
						jid: root.jid
					}
					delegate: MobileForm.AbstractFormDelegate {
						visible: deviceExpansionButton.checked
						background: Item {}
						contentItem: ColumnLayout {
							Controls.Label {
								text: {
									if (model.name) {
										if (model.version) {
											return model.name + " " + model.version
										}
										return model.name
									}
									return model.resource
								}
								textFormat: Text.PlainText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
							}

							Controls.Label {
								text: model.os
								color: Kirigami.Theme.disabledTextColor
								font: Kirigami.Theme.smallFont
								textFormat: Text.PlainText
								wrapMode: Text.WordWrap
								Layout.fillWidth: true
							}
						}
					}
				}

				FormExpansionButton {
					id: deviceExpansionButton
				}
			}
		}
	}

	function openKeyAuthenticationPage(keyAuthenticationPageComponent) {
		if (root.sheet) {
			root.sheet.close()
		}

		return openPage(keyAuthenticationPageComponent)
	}
}
