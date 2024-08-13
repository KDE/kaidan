// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

RosterItemDetailsContent {
	id: root

	required property string accountJid

	mediaOverview {
		accountJid: root.accountJid
		chatJid: root.jid
	}
	vCardRepeater {
		model: VCardModel {
			jid: root.jid
		}
		delegate: MobileForm.FormButtonDelegate {
			text: model.value
			description: model.key
			enabled: model.uriScheme === "mailto" || model.uriScheme === "http"
			onClicked: {
				if (model.uriScheme === "mailto") {
					Qt.openUrlExternally(model.uriScheme + ":" + model.value)
				} else if (model.uriScheme === "http") {
					Qt.openUrlExternally(model.value)
				}
			}
		}
	}
	encryptionArea: ColumnLayout {
		spacing: 0

		MobileForm.FormCardHeader {
			title: qsTr("Encryption")
		}

		MobileForm.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
			enabled: ChatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && ChatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: ChatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		}

		MobileForm.FormButtonDelegate {
			text: {
				if (!ChatController.accountEncryptionWatcher.hasUsableDevices) {
					if (ChatController.accountEncryptionWatcher.hasDistrustedDevices) {
						return qsTr("Verify <b>your</b> devices to encrypt for them")
					}

					if (ownResourcesWatcher.resourcesCount > 1) {
						return qsTr("<b>Your</b> other devices don't use OMEMO 2")
					}
				} else if (ChatController.accountEncryptionWatcher.hasAuthenticatableDevices) {
					if (ChatController.accountEncryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return qsTr("Verify <b>your</b> devices to encrypt for them")
					}

					return qsTr("Verify <b>your</b> devices for maximum security")
				}

				return ""
			}
			icon.name: {
				if (!ChatController.accountEncryptionWatcher.hasUsableDevices) {
					if (ChatController.accountEncryptionWatcher.hasDistrustedDevices) {
						return "channel-secure-symbolic"
					}

					if (ownResourcesWatcher.resourcesCount > 1) {
						return "channel-insecure-symbolic"
					}
				} else if (ChatController.accountEncryptionWatcher.hasAuthenticatableDevices) {
					if (ChatController.accountEncryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return "security-medium-symbolic"
					}

					return "security-high-symbolic"
				}

				return ""
			}
			visible: text
			enabled: ChatController.accountEncryptionWatcher.hasAuthenticatableDevices
			onClicked: root.openKeyAuthenticationPage(contactDetailsAccountKeyAuthenticationPage)

			UserResourcesWatcher {
				id: ownResourcesWatcher
				jid: root.accountJid
			}
		}

		MobileForm.FormButtonDelegate {
			text: {
				if (!ChatController.chatEncryptionWatcher.hasUsableDevices) {
					if (ChatController.chatEncryptionWatcher.hasDistrustedDevices) {
						return qsTr("Verify your <b>contact</b> to enable encryption")
					}

					return qsTr("Your <b>contact</b> doesn't use OMEMO 2")
				}

				if (ChatController.chatEncryptionWatcher.hasAuthenticatableDevices) {
					if (ChatController.chatEncryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return qsTr("Verify your <b>contact's</b> devices to encrypt for them")
					}

					return qsTr("Verify your <b>contact</b> for maximum security")
				}

				return ""
			}
			icon.name: {
				if (!ChatController.chatEncryptionWatcher.hasUsableDevices) {
					if (ChatController.chatEncryptionWatcher.hasDistrustedDevices) {
						return "channel-secure-symbolic"
					}

					return "channel-insecure-symbolic"
				}

				if (ChatController.chatEncryptionWatcher.hasAuthenticatableDevices) {
					if (ChatController.chatEncryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return "security-medium-symbolic"
					}

					return "security-high-symbolic"
				}

				return ""
			}
			visible: text
			enabled: ChatController.chatEncryptionWatcher.hasAuthenticatableDevices
			onClicked: root.openKeyAuthenticationPage(contactDetailsKeyAuthenticationPage)
		}
	}
	qrCodeExpansionButton.description: qsTr("Share this contact's chat address via QR code")
	qrCode: ContactQrCode {
		accountJid: root.accountJid
		jid: root.jid
	}
	qrCodeButton {
		description: qsTr("Share this contact's chat address via QR code")
		onClicked: Utils.copyToClipboard(qrCode.source)
	}
	uriButton {
		description: qsTr("Share this contact's chat address via text")
		onClicked: {
			Utils.copyToClipboard(trustMessageUriGenerator.uri)
			passiveNotification(qsTr("Contact copied to clipboard"))
		}
	}
	invitationButton {
		description: qsTr("Share this contact's chat address via a web page with usage help")
		onClicked: Utils.copyToClipboard(Utils.invitationUrl(trustMessageUriGenerator.uri))
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Notifications")
			}

			MobileForm.FormComboBoxDelegate {
				id: notificationDelegate
				text: qsTr("Incoming messages")
				description: qsTr("Show notification and play sound on message arrival")
				model: [
					{
						display: qsTr("Account default"),
						value: RosterItem.NotificationRule.Account
					},
					{
						display: qsTr("Never"),
						value: RosterItem.NotificationRule.Never
					},
					{
						display: qsTr("Always"),
						value: RosterItem.NotificationRule.Always
					}
				]
				textRole: "display"
				valueRole: "value"
				currentIndex: notificationDelegate.indexOf(rosterItemWatcher.item.notificationRule)
				onActivated: RosterModel.setNotificationRule(root.accountJid, root.jid, notificationDelegate.currentValue)

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
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Privacy")
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Request personal data")
				description: qsTr("Ask your contact to share the availability, devices and other personal information")
				visible: Kaidan.connectionState === Enums.StateConnected && !root.rosterItemWatcher.item.sendingPresence
				onClicked: Kaidan.client.rosterManager.subscribeToPresenceRequested(root.jid)
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Cancel personal data sharing")
				description: qsTr("Stop sharing your availability, devices and other personal information")
				visible: Kaidan.connectionState === Enums.StateConnected && root.rosterItemWatcher.item.receivingPresence
				onClicked: Kaidan.client.rosterManager.refuseSubscriptionToPresenceRequested(root.jid)
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send typing notifications")
				description: qsTr("Indicate when you have this conversation open, are typing and stopped typing")
				checked: root.rosterItemWatcher.item.chatStateSendingEnabled
				onToggled: {
					RosterModel.setChatStateSendingEnabled(
						root.accountJid,
						root.jid,
						checked)
				}
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send read notifications")
				description: qsTr("Indicate which messages you have read")
				checked: root.rosterItemWatcher.item.readMarkerSendingEnabled
				onToggled: {
					RosterModel.setReadMarkerSendingEnabled(
						root.accountJid,
						root.jid,
						checked)
				}
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Block")
				description: qsTr("Block all communication including status and notifications")
				enabled: !blockingAction.loading && Kaidan.connectionState === Enums.StateConnected
				checked: blockingWatcher.blocked
				onToggled: {
					if (checked) {
						blockingAction.block(root.jid)
					} else {
						blockingAction.unblock(root.jid)
					}
				}

				BlockingWatcher {
					id: blockingWatcher
					jid: root.jid
				}
			}
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			id: removalArea
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Removal")
			}

			ColumnLayout {
				spacing: 0

				MobileForm.FormButtonDelegate {
					id: contactRemovalButton
					text: qsTr("Remove")
					description: qsTr("Remove contact and complete chat history")
					icon.name: "edit-delete-symbolic"
					icon.color: "red"
					onClicked: contactRemovalConfirmationButton.visible = !contactRemovalConfirmationButton.visible
				}

				MobileForm.FormButtonDelegate {
					id: contactRemovalConfirmationButton
					text: qsTr("Confirm")
					visible: false
					Layout.leftMargin: Kirigami.Units.largeSpacing * 6
					onClicked: {
						visible = false
						contactRemovalButton.enabled = false

						Kaidan.client.rosterManager.removeContactRequested(jid)
					}
				}
			}
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		accountJid: root.accountJid
		jid: root.jid
	}
}
