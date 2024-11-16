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
	mediaOverview {
		accountJid: ChatController.accountJid
		chatJid: ChatController.chatJid
	}
	vCardRepeater {
		model: VCardModel {
			jid: ChatController.chatJid
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

		AccountKeyAuthenticationButton {
			jid: ChatController.accountJid
			encryptionWatcher: ChatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsAccountKeyAuthenticationPage)
		}

		ContactKeyAuthenticationButton {
			encryptionWatcher: ChatController.chatEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsKeyAuthenticationPage)
		}
	}
	qrCodeExpansionButton.description: qsTr("Share this contact's chat address via QR code")
	qrCode: ContactQrCode {
		accountJid: ChatController.accountJid
		jid: ChatController.chatJid
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

	UserDevicesArea {
		jid: ChatController.chatJid
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Notifications")
			}

			FormComboBoxDelegate {
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
				currentIndex: indexOf(ChatController.rosterItem.notificationRule)
				onActivated: RosterModel.setNotificationRule(ChatController.accountJid, ChatController.chatJid, currentValue)
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
				visible: Kaidan.connectionState === Enums.StateConnected && !ChatController.rosterItem.sendingPresence
				onClicked: Kaidan.client.rosterManager.subscribeToPresenceRequested(ChatController.chatJid)
			}

			MobileForm.FormButtonDelegate {
				text: qsTr("Cancel personal data sharing")
				description: qsTr("Stop sharing your availability, devices and other personal information")
				visible: Kaidan.connectionState === Enums.StateConnected && ChatController.rosterItem.receivingPresence
				onClicked: Kaidan.client.rosterManager.refuseSubscriptionToPresenceRequested(ChatController.chatJid)
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send typing notifications")
				description: qsTr("Indicate when you have this conversation open, are typing and stopped typing")
				checked: ChatController.rosterItem.chatStateSendingEnabled
				onToggled: {
					RosterModel.setChatStateSendingEnabled(
						ChatController.accountJid,
						ChatController.chatJid,
						checked)
				}
			}

			MobileForm.FormSwitchDelegate {
				text: qsTr("Send read notifications")
				description: qsTr("Indicate which messages you have read")
				checked: ChatController.rosterItem.readMarkerSendingEnabled
				onToggled: {
					RosterModel.setReadMarkerSendingEnabled(
						ChatController.accountJid,
						ChatController.chatJid,
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
						blockingAction.block(ChatController.chatJid)
					} else {
						blockingAction.unblock(ChatController.chatJid)
					}
				}

				BlockingWatcher {
					id: blockingWatcher
					jid: ChatController.chatJid
				}
			}
		}
	}

	MobileForm.FormCard {
		Layout.fillWidth: true

		contentItem: ColumnLayout {
			spacing: 0

			MobileForm.FormCardHeader {
				title: qsTr("Removal")
			}

			ConfirmationFormButtonArea {
				button {
					text: qsTr("Remove")
					description: qsTr("Remove contact and complete chat history")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.negativeTextColor
				}
				confirmationButton.onClicked: {
					busy = true
					Kaidan.client.rosterManager.removeContactRequested(ChatController.chatJid)
				}
				busyText: qsTr("Removing contact…")
			}
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		accountJid: ChatController.accountJid
		jid: ChatController.chatJid
	}
}
