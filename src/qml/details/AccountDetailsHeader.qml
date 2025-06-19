// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

DetailsHeader {
	id: root
	jid: account.settings.jid
	displayName: account.settings.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Change your profile image")
		icon.name: "camera-photo-symbolic"
		enabled: root.account.settings.enabled
		onTriggered: {
			if (root.dialog) {
				root.dialog.close()
			}

			openPage(avatarChangePage)
		}
	}

	function changeDisplayName(newDisplayName) {
		account.vCardController.changeNickname(newDisplayName)
	}

	function handleDisplayNameChanged() {
		if (account.connection.state === Enums.StateConnected) {
			account.vCardController.requestOwnVCard()
		}
	}
}
