// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts

import im.kaidan.kaidan

import "fields"

ConfirmationArea {
	id: root
	property Account account
	property alias groupChatJidField: groupChatJidField
	property alias groupChatJid: groupChatJidField.text

	confirmationButton.text: qsTr("Join")
	confirmationButton.onClicked: joinGroupChat()
	loadingArea.description: qsTr("Joining group chat…")
	busy: account.groupChatController.busy

	JidField {
		id: groupChatJidField
		labelText: qsTr("Address")
		placeholderText: qsTr("group@groups.example.org")
		inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhPreferLowercase
		invalidHintText: qsTr("The address must have the form <b>group@server</b>")
		Layout.fillWidth: true
		inputField.onAccepted: valid ? nicknameField.forceActiveFocus() : forceActiveFocus()
	}

	Field {
		id: nicknameField
		labelText: qsTr("Nickname (optional):")
		text: root.account.settings.displayName
		inputMethodHints: Qt.ImhPreferUppercase
		Layout.fillWidth: true
		inputField.onAccepted: joinGroupChat()
	}

	Connections {
		target: root.account.groupChatController

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be joined%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function joinGroupChat() {
		if (groupChatJidField.valid) {
			account.groupChatController.joinGroupChat(groupChatJid, nicknameField.text)
		} else {
			groupChatJidField.forceActiveFocus()
		}
	}
}
