// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

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
		delegate: FormCard.FormButtonDelegate {
			text: model.value
			description: model.key
			visible: !vCardExpansionButton.visible || vCardExpansionButton.checked
			enabled: model.uriScheme === "mailto" || model.uriScheme === "http"
			background: FormCard.FormDelegateBackground {
				control: parent
				// Needed since the corners would otherwise not be rounded because of the repeater.
				corners {
					bottomLeftRadius: vCardExpansionButton.visible ? 0 : Kirigami.Units.smallSpacing
					bottomRightRadius: vCardExpansionButton.visible ? 0 : Kirigami.Units.smallSpacing
				}
			}
			onClicked: {
				if (model.uriScheme === "mailto") {
					Qt.openUrlExternally(model.uriScheme + ":" + model.value)
				} else if (model.uriScheme === "http") {
					Qt.openUrlExternally(model.value)
				}
			}
		}
	}
	encryptionArea.delegates: [
		FormCard.FormHeader {
			title: qsTr("Encryption")
		},

		FormCard.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you and your chat partners can read or modify the data you exchange.")
			enabled: ChatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && ChatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: ChatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		},

		AccountKeyAuthenticationButton {
			jid: ChatController.accountJid
			encryptionWatcher: ChatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsAccountKeyAuthenticationPage)
		},

		ContactKeyAuthenticationButton {
			encryptionWatcher: ChatController.chatEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsKeyAuthenticationPage)
		}
	]
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

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
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

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Privacy")
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Request personal data")
			description: qsTr("Ask your contact to share the availability, devices and other personal information")
			visible: Kaidan.connectionState === Enums.StateConnected && !ChatController.rosterItem.sendingPresence
			onClicked: Kaidan.rosterController.subscribeToPresence(ChatController.chatJid)
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Cancel personal data sharing")
			description: qsTr("Stop sharing your availability, devices and other personal information")
			visible: Kaidan.connectionState === Enums.StateConnected && ChatController.rosterItem.receivingPresence
			onClicked: Kaidan.rosterController.refuseSubscriptionToPresence(ChatController.chatJid)
		}

		FormCard.FormSwitchDelegate {
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

		FormCard.FormSwitchDelegate {
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

		FormCard.FormSwitchDelegate {
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

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
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
				Kaidan.rosterController.removeContact(ChatController.chatJid)
			}
			busyText: qsTr("Removing contact…")
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		accountJid: ChatController.accountJid
		jid: ChatController.chatJid
	}
}
