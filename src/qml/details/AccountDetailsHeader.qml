// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

DetailsHeader {
	id: root

	property Kirigami.Dialog dialog

	displayName: AccountController.account.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Change your profile image")
		icon.name: "camera-photo-symbolic"
		onTriggered: {
			if (root.dialog) {
				root.dialog.close()
			}

			openPage(avatarChangePage)
		}
	}

	function changeDisplayName(newDisplayName) {
		Kaidan.client.vCardManager.changeNicknameRequested(newDisplayName)
	}

	function handleDisplayNameChanged() {
		if (Kaidan.connectionState === Enums.StateConnected) {
			Kaidan.client.vCardManager.clientVCardRequested()
		}
	}
}
