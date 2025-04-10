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
		chatJid: ChatController.accountJid
	}
	encryptionArea.delegates: [
		FormCard.FormHeader {
			title: qsTr("Encryption")
		},

		FormCard.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you can read or modify the data that is synchronized across your devices.")
			enabled: ChatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && ChatController.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: ChatController.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		},

		AccountKeyAuthenticationButton {
			jid: ChatController.accountJid
			encryptionWatcher: ChatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(notesChatDetailsKeyAuthenticationPage)
		}
	]
	sharingArea.visible: false

	FormCard.FormCard {
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Removal")
		}

		ConfirmationFormButtonArea {
			button {
				text: qsTr("Remove")
				description: qsTr("Remove notes chat and complete chat history")
				icon.name: "edit-delete-symbolic"
				icon.color: Kirigami.Theme.negativeTextColor
			}
			confirmationButton.onClicked: {
				busy = true
				Kaidan.rosterController.removeContact(ChatController.accountJid)
			}
			busyText: qsTr("Removing notes chat…")
		}
	}

	ContactTrustMessageUriGenerator {
		id: trustMessageUriGenerator
		accountJid: ChatController.accountJid
		jid: ChatController.accountJid
	}
}
