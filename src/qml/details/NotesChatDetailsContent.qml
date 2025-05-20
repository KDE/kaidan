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
		chatJid: root.chatController.account.settings.jid
	}
	encryptionArea.delegates: [
		FormCard.FormHeader {
			title: qsTr("Encryption")
		},

		FormCard.FormSwitchDelegate {
			text: qsTr("OMEMO 2")
			description: qsTr("End-to-end encryption with OMEMO 2 ensures that nobody else than you can read or modify the data that is synchronized across your devices.")
			enabled: root.chatController.chatEncryptionWatcher.hasUsableDevices
			checked: enabled && root.chatController.account.settings.encryption === Encryption.Omemo2
			// The switch is toggled by setting the user's preference on using encryption.
			// Note that 'checked' has already the value after the button is clicked.
			onClicked: root.chatController.account.settings.encryption = checked ? Encryption.Omemo2 : Encryption.NoEncryption
		},

		AccountKeyAuthenticationButton {
			presenceCache: root.chatController.account.presenceCache
			jid: root.chatController.account.settings.jid
			encryptionWatcher: root.chatController.accountEncryptionWatcher
			onClicked: root.openKeyAuthenticationPage(notesChatDetailsKeyAuthenticationPage)
		}
	]
	rosterGoupListView.header: null
	sharingArea.visible: false

	FormCard.FormCard {
		visible: root.chatController.account.settings.enabled
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
				root.chatController.account.rosterController.removeContact(root.chatController.account.settings.jid)
			}
			busyText: qsTr("Removing notes chat…")
		}
	}
}
