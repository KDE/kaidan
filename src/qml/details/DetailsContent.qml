// SPDX-FileCopyrightText: 2023 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import ".."
import "../elements"

FormInfoContent {
	id: root

	property Account account
	property alias topArea: topArea.data
	property alias automaticMediaDownloadsDelegate: automaticMediaDownloadsDelegate
	property alias mediaAreaText: mediaAreaText.text
	property alias mediaOverview: mediaOverview
	property alias mediaOverviewExpansionButton: mediaOverviewExpansionButton
	property alias vCardArea: vCardArea
	property alias vCardRepeater: vCardRepeater
	property alias vCardExpansionButton: vCardExpansionButton
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
		visible: children.length
	}

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Media")
		}

		FormCard.FormSectionText {
			id: mediaAreaText
			visible: text
		}

		FormComboBoxDelegate {
			id: automaticMediaDownloadsDelegate
			text: qsTr("Automatic downloads")
			description: qsTr("Download media automatically")
		}

		ColumnLayout {
			visible: mediaOverviewExpansionButton.visible && mediaOverviewExpansionButton.checked
			spacing: 0

			Kirigami.Separator {
				Layout.fillWidth: true
			}

			MediaOverview {
				id: mediaOverview
				Layout.fillWidth: true
				Component.onCompleted: loadDownloadedFiles()
			}
		}

		FormExpansionButton {
			id: mediaOverviewExpansionButton
			visible: mediaOverview.totalFilesCount
			onCheckedChanged: {
				if (checked) {
					mediaOverview.selectionMode = false

					// Display the content of the first tab only on initial loading.
					// Afterwards, display the content of the last active tab.
					if (mediaOverview.tabBarCurrentIndex === -1) {
						mediaOverview.tabBarCurrentIndex = 0
					}
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

		FormExpansionButton {
			id: vCardExpansionButton
			background: FormCard.FormDelegateBackground {
				control: parent
				// Needed since the corners would otherwise not be rounded because of the repeater.
				corners {
					bottomLeftRadius: vCardArea.cardWidthRestricted ? Kirigami.Units.smallSpacing : 0
					bottomRightRadius: vCardArea.cardWidthRestricted ? Kirigami.Units.smallSpacing : 0
				}
			}
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

		InlineListView {
			id: rosterGoupListView
			model: root.account.rosterController.groups
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
			checkable: true
		}

		FormCard.AbstractFormDelegate {
			id: qrCodeArea
			visible: qrCodeExpansionButton.checked
			background: FormCard.FormDelegateBackground {
				control: parent
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
