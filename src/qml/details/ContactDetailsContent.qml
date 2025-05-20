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
		accountJid: root.chatController.account.settings.jid
		chatJid: root.chatController.jid
	}
	vCardRepeater {
		model: VCardModel {
			connection: root.chatController.account.connection
			vCardController: root.chatController.account.vCardController
			jid: root.chatController.jid
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
			enabled: root.chatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && root.chatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: root.chatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		},

		AccountKeyAuthenticationButton {
			presenceCache: root.chatController.account.presenceCache
			jid: root.chatController.account.settings.jid
			encryptionWatcher: root.chatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsAccountKeyAuthenticationPage)
		},

		ContactKeyAuthenticationButton {
			encryptionWatcher: root.chatController.chatEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(contactDetailsKeyAuthenticationPage)
		}
	]
	qrCodeExpansionButton.description: qsTr("Share this contact's chat address via QR code")
	qrCode: ContactQrCode {
		uriGenerator: trustMessageUriGenerator
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
		account: root.chatController.account
		jid: root.chatController.jid
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
			currentIndex: indexOf(root.chatController.rosterItem.notificationRule)
			onActivated: root.chatController.account.rosterController.setNotificationRule(root.chatController.jid, currentValue)
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
			visible: root.chatController.account.settings.enabled && root.chatController.account.connection.state === Enums.StateConnected && !root.chatController.rosterItem.sendingPresence
			onClicked: root.chatController.account.rosterController.subscribeToPresence(root.chatController.jid)
		}

		FormCard.FormButtonDelegate {
			text: qsTr("Cancel personal data sharing")
			description: qsTr("Stop sharing your availability, devices and other personal information")
			visible: root.chatController.account.settings.enabled && root.chatController.account.connection.state === Enums.StateConnected && root.chatController.rosterItem.receivingPresence
			onClicked: root.chatController.account.rosterController.refuseSubscriptionToPresence(root.chatController.jid)
		}

		FormCard.FormSwitchDelegate {
			text: qsTr("Send typing notifications")
			description: qsTr("Indicate when you have this conversation open, are typing and stopped typing")
			checked: root.chatController.rosterItem.chatStateSendingEnabled
			onToggled: root.chatController.account.rosterController.setChatStateSendingEnabled(root.chatController.jid, checked)
		}

		FormCard.FormSwitchDelegate {
			text: qsTr("Send read notifications")
			description: qsTr("Indicate which messages you have read")
			checked: root.chatController.rosterItem.readMarkerSendingEnabled
			onToggled: root.chatController.account.rosterController.setReadMarkerSendingEnabled(root.chatController.jid, checked)
		}

		FormCard.FormSwitchDelegate {
			text: qsTr("Block")
			description: qsTr("Block all communication including status and notifications")
			visible: root.chatController.account.settings.enabled
			enabled: !root.chatController.account.blockingController.busy && root.chatController.account.connection.state === Enums.StateConnected
			checked: blockingWatcher.blocked
			onToggled: {
				if (checked) {
					root.chatController.account.blockingController.block(root.chatController.jid)
				} else {
					root.chatController.account.blockingController.unblock(root.chatController.jid)
				}
			}

			BlockingWatcher {
				id: blockingWatcher
				blockingController: root.chatController.account.blockingController
				jid: root.chatController.jid
			}
		}
	}

	FormCard.FormCard {
		visible: root.chatController.account.settings.enabled
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
				root.chatController.account.rosterController.removeContact(root.chatController.jid)
			}
			busyText: qsTr("Removing contact…")
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		encryptionController: root.chatController.account.encryptionController
		jid: root.chatController.jid
	}
}
