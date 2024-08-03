// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.Dialog {
	id: root

	property alias accountJid: content.accountJid
	property alias groupChatJid: content.groupChatJid
	property alias nickname: content.nickname

	title: qsTr("Join group chat")
	standardButtons: Kirigami.Dialog.NoButton
	padding: Kirigami.Units.mediumSpacing
	preferredWidth: largeButtonWidth
	onOpened: content.groupChatJidField.forceActiveFocus()
	onClosed: destroy()

	GroupChatJoiningContent {
		id: content
		groupChatJidField.inputField.onActiveFocusChanged: {
			// The active focus is taken by another item after opening.
			// Thus, it must be forced again.
			if (!groupChatJidField.inputField.activeFocus && !nicknameField.inputField.activeFocus) {
				groupChatJidField.forceActiveFocus()
				groupChatJidField.invalidHintMayBeShown = false
			} else {
				groupChatJidField.invalidHintMayBeShown = true
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
