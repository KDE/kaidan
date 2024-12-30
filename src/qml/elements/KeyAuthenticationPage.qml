// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

/**
 * This page is used for authenticating encryption keys by scanning QR codes or entering key IDs.
 */
ExplanationOptionsTogglePage {
	id: root

	property string accountJid
	property string jid
	property alias authenticatableKeysArea: authenticatableKeysArea

	title: qsTr("Verify devices")
	explanationInitiallyVisible: Kaidan.settings.keyAuthenticationPageExplanationVisible
	primaryButton.text: state === "primaryAreaDisplayed" ? qsTr("Show explanation") : qsTr("Scan QR codes")
	primaryButton.onClicked: {
		if (Kaidan.settings.keyAuthenticationPageExplanationVisible) {
			// Hide the explanation when this page is opened again in the future.
			Kaidan.settings.keyAuthenticationPageExplanationVisible = false

			if (!qrCodeScanningArea.scanner.cameraEnabled) {
				qrCodeScanningArea.scanner.cameraEnabled = true
			}
		}
	}
	secondaryButton.text: state === "secondaryAreaDisplayed" ? qsTr("Show explanation") : qsTr("Verify manually")
	primaryArea: QrCodeScanningArea {
		id: qrCodeScanningArea
		accountJid: root.accountJid
		jid: root.jid
		visible: !Kaidan.settings.keyAuthenticationPageExplanationVisible
		anchors.centerIn: parent
	}
	secondaryArea: GridLayout {
		anchors.fill: parent
		flow: parent.width > parent.height ? GridLayout.LeftToRight : GridLayout.TopToBottom
		rowSpacing: Kirigami.Units.largeSpacing * 2
		columnSpacing: rowSpacing

		EncryptionKeysArea {
			id: authenticatableKeysArea
			listView.header: FormCard.FormCard {
				width: ListView.view.width
				Kirigami.Theme.colorSet: Kirigami.Theme.Window

				FormCard.AbstractFormDelegate {
					background: null
					contentItem:RowLayout {
						spacing: Kirigami.Units.largeSpacing * 2

						Controls.TextField {
							id: encryptionKeyField
							onTextChanged: text = Utils.displayableEncryptionKeyId(text)
							placeholderText: "899bdd30 74f346c3 34cec4bb 536be448 c33886f3 057a912a 1f299b0f 32193d6c"
							inputMethodHints: Qt.ImhPreferLatin | Qt.ImhPreferLowercase | Qt.ImhLatinOnly
							enabled: !encryptionKeyBusyIndicator.visible
							Layout.fillWidth: true
							onAccepted: encryptionKeyAuthenticationButton.clicked()
							onVisibleChanged: {
								if (visible) {
									forceActiveFocus()
								}
							}
						}

						Button {
							id: encryptionKeyAuthenticationButton
							Controls.ToolTip.text: qsTr("Verify device")
							icon.name: "emblem-ok-symbolic"
							visible: !encryptionKeyBusyIndicator.visible
							flat: !hovered
							Layout.preferredWidth: Layout.preferredHeight
							Layout.preferredHeight: encryptionKeyField.implicitHeight
							onClicked: {
								// Remove empty spaces from the key ID and convert it to lower case.
								const keyId = encryptionKeyField.text.replace(/\s/g, "").toLowerCase()

								if (Utils.validateEncryptionKeyId(keyId)) {
									encryptionKeyBusyIndicator.visible = true

									Kaidan.client.atmManager.makeTrustDecisionsRequested(root.jid, [keyId], [])
									encryptionKeyField.clear()
									passiveNotification(qsTr("Device verified"))

									encryptionKeyBusyIndicator.visible = false
								} else {
									passiveNotification(qsTr("The fingerprint must have 64 characters with digits and letters from a to f"))
								}

								encryptionKeyField.forceActiveFocus()
							}
						}

						Controls.BusyIndicator {
							id: encryptionKeyBusyIndicator
							visible: false
							Layout.preferredWidth: encryptionKeyAuthenticationButton.Layout.preferredWidth
							Layout.preferredHeight: Layout.preferredWidth
							Layout.rightMargin: encryptionKeyAuthenticationButton.Layout.rightMargin
						}
					}
				}
			}
			listView.delegate: FormCard.FormTextDelegate {
				text: model.label
				description: "`" + Utils.displayableEncryptionKeyId(model.keyId) + "`"
				descriptionItem.textFormat: Text.MarkdownText
				width: ListView.view.width
				horizontalPadding: Kirigami.Units.smallSpacing * 6.5
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

		EncryptionKeysArea {
			header.title: qsTr("Verified own devices")
			listView.model: AuthenticatedEncryptionKeyModel {
				accountJid: root.accountJid
			}
			listView.delegate: FormCard.AbstractFormDelegate {
				id: encryptionKeyDelegate
				width: ListView.view.width
				leftPadding: 0
				verticalPadding: 0
				contentItem: RowLayout {
					FormCard.FormTextDelegate {
						id: encryptionKeyText
						text: model.label
						description: "`" + Utils.displayableEncryptionKeyId(model.keyId) + "`"
						descriptionItem.textFormat: Text.MarkdownText
						Layout.fillWidth: true
						onClicked: encryptionKeyCopyButton.clicked()
					}

					Button {
						id: encryptionKeyCopyButton
						text: qsTr("Copy fingerprint")
						icon.name: "edit-copy-symbolic"
						display: Controls.AbstractButton.IconOnly
						flat: !hovered && !encryptionKeyDelegate.hovered
						Controls.ToolTip.text: text
						onClicked: {
							Utils.copyToClipboard(model.keyId)
							passiveNotification(qsTr("Fingerprint copied to clipboard"))
						}
					}
				}
			}
		}
	}

	Component.onCompleted: {
		if (!Kaidan.settings.keyAuthenticationPageExplanationVisible) {
			qrCodeScanningArea.scanner.cameraEnabled = true
		}
	}
}
