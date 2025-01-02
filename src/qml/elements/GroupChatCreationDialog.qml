// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Dialog {
	id: root

	property alias accountJid: content.accountJid
	property alias groupChatName: content.groupChatName
	property alias nickname: content.nickname

	title: qsTr("Create group chat")
	padding: Kirigami.Units.mediumSpacing
	onOpened: content.groupChatNameField.forceActiveFocus()

	GroupChatCreationContent {
		id: content
		groupChatNameField.inputField.onActiveFocusChanged: {
			// The active focus is taken by another item after opening.
			// Thus, it must be forced again.
			if (!groupChatNameField.inputField.activeFocus && !groupChatIdField.inputField.activeFocus && !nicknameField.inputField.activeFocus) {
				groupChatNameField.forceActiveFocus()
				groupChatNameField.invalidHintMayBeShown = false
			} else {
				groupChatNameField.invalidHintMayBeShown = true
			}
		}
	}

	Connections {
		target: Kaidan

		function onOpenChatPageRequested(accountJid, chatJid) {
			root.close()
		}
	}
}
