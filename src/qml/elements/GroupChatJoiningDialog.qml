// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias account: content.account
	property alias groupChatJid: content.groupChatJid

	title: qsTr("Join group chat")
	padding: Kirigami.Units.mediumSpacing
	onOpened: content.groupChatJidField.forceActiveFocus()

	GroupChatJoiningContent {
		id: content
	}

	Connections {
		target: root.account.groupChatController

		function onGroupChatJoined(groupChatJid) {
			if (groupChatJid === root.groupChatJid) {
				root.close()
			}
		}
	}
}
