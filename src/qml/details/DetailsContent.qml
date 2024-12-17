// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import QtQuick.Layouts 1.15
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan 1.0

import ".."
import "../elements"

FormInfoContent {
	id: root

	property Kirigami.Dialog dialog
	property alias topArea: topArea.data
	property alias automaticMediaDownloadsDelegate: automaticMediaDownloadsDelegate
	property alias mediaOverview: mediaOverview
	property alias mediaOverviewExpansionButton: mediaOverviewExpansionButton
	property alias vCardArea: vCardArea
	property alias vCardRepeater: vCardRepeater
	property alias rosterGoupListView: rosterGoupListView
	property alias sharingArea: sharingArea
	property alias qrCodeExpansionButton: qrCodeExpansionButton
	property alias qrCode: qrCodeArea.contentItem
	property alias qrCodeButton: qrCodeButton
	property alias uriButton: uriButton
	property alias invitationButton: invitationButton
	property alias encryptionArea: encryptionArea

	enum EncryptionKeyAuthenticationState {
		DistrustedKeysOnly,
		ResourcesWithoutKeys,
		NoKeys,
		AuthenticatableDistrustedKeys,
		AuthenticatableTrustedKeysOnly,
		AllKeysAuthenticated
	}

	ColumnLayout {
		id: topArea
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Media")
		}

		FormComboBoxDelegate {
			id: automaticMediaDownloadsDelegate
			text: qsTr("Automatic downloads")
			description: qsTr("Download media automatically")
		}

		ColumnLayout {
			visible: mediaOverviewExpansionButton.visible && mediaOverviewExpansionButton.checked

			FormCard.FormSectionText {
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

	FormCard.FormCard {
		id: vCardArea
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Profile")
		}

		Repeater {
			id: vCardRepeater
			Layout.fillHeight: true
		}
	}

	FormCard.FormCard {
		id: encryptionArea
		Layout.fillWidth: true
	}

	FormCard.FormCard {
		// Hide this if there are no items and no header.
		visible: rosterGoupListView.count || rosterGoupListView.headerItem
		Layout.fillWidth: true

		FormCard.FormHeader {
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

	FormCard.FormCard {
		id: sharingArea
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Sharing")
		}

		FormCard.FormButtonDelegate {
			id: qrCodeExpansionButton
			text: qsTr("Show QR code")
			icon.name: "view-barcode-qr"
			// TODO: If possible, scroll down to show whole QR code
			onClicked: qrCodeArea.visible = !qrCodeArea.visible
		}

		FormCard.AbstractFormDelegate {
			id: qrCodeArea
			visible: false
			background: Rectangle {
				color: secondaryBackgroundColor
			}
			Layout.preferredWidth: parent.width
			Layout.preferredHeight: Layout.preferredWidth
		}

		FormCard.FormButtonDelegate {
			id: qrCodeButton
			text: qsTr("Copy QR code")
			icon.name: "send-to-symbolic"
			onClicked: passiveNotification(qsTr("QR code copied to clipboard"))
		}

		FormCard.FormButtonDelegate {
			id: uriButton
			text: qsTr("Copy chat address")
			icon.name: "send-to-symbolic"
		}

		FormCard.FormButtonDelegate {
			id: invitationButton
			text: qsTr("Copy invitation link")
			icon.name: "mail-message-new-symbolic"
			onClicked: passiveNotification(qsTr("Invitation link copied to clipboard"))
		}
	}

	function openKeyAuthenticationPage(keyAuthenticationPageComponent) {
		if (root.dialog) {
			root.dialog.close()
		}

		return openPage(keyAuthenticationPageComponent)
	}
}
