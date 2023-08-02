// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

DetailsHeader {
	id: root

	property Kirigami.OverlaySheet sheet

	displayName: AccountManager.displayName
	avatarAction: Kirigami.Action {
		text: qsTr("Change your profile image")
		icon.name: "camera-photo-symbolic"
		onTriggered: pageStack.layers.push(avatarChangePage)
	}

	Component {
		id: avatarChangePage

		AvatarChangePage {
			Component.onCompleted: {
				if (root.sheet) {
					root.sheet.close()
				}
			}

			Component.onDestruction: {
				if (root.sheet) {
					root.sheet.open()
					Kaidan.client.vCardManager.clientVCardRequested()
				}
			}
		}
	}

	function changeDisplayName(newDisplayName) {
		Kaidan.client.vCardManager.changeNicknameRequested(newDisplayName)
	}

	function handleDisplayNameChanged() {
		if (Kaidan.connectionState === Enums.StateConnected) {
			Kaidan.client.vCardManager.clientVCardRequested(root.jid)
		}
	}
}
