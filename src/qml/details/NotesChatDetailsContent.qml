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
		chatJid: ChatController.accountJid
	}
	encryptionArea: ColumnLayout {
		spacing: 0

		MobileForm.FormCardHeader {
			title: qsTr("Encryption")
		}

		MobileForm.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you can read or modify the data that is synchronized across your devices.")
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

					return qsTr("<b>You</b> have no other devices supporting OMEMO 2")
				}

				if (ChatController.accountEncryptionWatcher.hasAuthenticatableDevices) {
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

					return "channel-insecure-symbolic"
				}

				if (ChatController.accountEncryptionWatcher.hasAuthenticatableDevices) {
					if (ChatController.accountEncryptionWatcher.hasAuthenticatableDistrustedDevices) {
						return "security-medium-symbolic"
					}

					return "security-high-symbolic"
				}

				return ""
			}
			visible: text
			enabled: ChatController.accountEncryptionWatcher.hasAuthenticatableDevices
			onClicked: root.openKeyAuthenticationPage(notesChatDetailsAccountKeyAuthenticationPage)

			UserResourcesWatcher {
				id: ownResourcesWatcher
				jid: ChatController.accountJid
			}
		}
	}
	sharingArea.visible: false

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
					description: qsTr("Remove notes chat and complete chat history")
					icon.name: "edit-delete-symbolic"
					icon.color: Kirigami.Theme.negativeTextColor
				}
				confirmationButton.onClicked: Kaidan.client.rosterManager.removeContactRequested(ChatController.accountJid)
				busyText: qsTr("Removing notes chat…")
			}
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		accountJid: ChatController.accountJid
		jid: ChatController.accountJid
	}
}
