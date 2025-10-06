// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import "../elements"

RosterItemDetailsContent {
	id: root
	mediaOverview {
		accountJid: root.chatController.account.settings.jid
		chatJid: root.chatController.account.settings.jid
	}
	rosterGoupListView.header: null
	sharingArea.visible: false
	encryptionArea.visible: false

	FormCard.FormCard {
		visible: root.chatController.account.settings.enabled
		Layout.fillWidth: true

		FormCard.FormHeader {
			title: qsTr("Removal")
		}

		ConfirmationFormButtonArea {
			button {
				text: qsTr("Remove")
				description: qsTr("Remove provider chat and complete chat history")
				icon.name: "edit-delete-symbolic"
				icon.color: Kirigami.Theme.negativeTextColor
			}
			confirmationButton.onClicked: {
				busy = true
				root.chatController.account.rosterController.removeContact(root.chatController.jid)
			}
			busyText: qsTr("Removing provider chat…")
		}
	}
}
