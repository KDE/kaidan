// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

DetailsHeader {
	id: root

	property ChatController chatController

	account: chatController.account
	jid: chatController.jid
	displayName: chatController.rosterItem.displayName
	changeDisplayName: (newDisplayName) => {
		account.rosterController.renameContact(chatController.jid, newDisplayName)
	}
	avatarAction: Kirigami.Action {
		text: qsTr("Maximize avatar")
		icon.name: "view-fullscreen-symbolic"
		enabled: root.avatar.source.toString()
		onTriggered: Qt.openUrlExternally(root.avatar.source)
	}
}
