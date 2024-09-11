// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

import "fields"

ConfirmationArea {
	id: root
	required property string accountJid
	property alias groupChatJidField: groupChatJidField
	property alias groupChatJid: groupChatJidField.text
	property alias nicknameField: nicknameField
	property alias nickname: nicknameField.text

	confirmationButton.text: qsTr("Join")
	confirmationButton.onClicked: joinGroupChat()
	loadingArea.description: qsTr("Joining group chat…")
	busy: GroupChatController.busy

	JidField {
		id: groupChatJidField
		labelText: qsTr("Address")
		text: ""
		placeholderText: qsTr("group@groups.example.org")
		inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhPreferLowercase
		invalidHintText: qsTr("The address must have the form <b>group@server</b>")
		Layout.fillWidth: true
		inputField.onAccepted: valid ? nicknameField.forceActiveFocus() : forceActiveFocus()
	}

	Field {
		id: nicknameField
		labelText: qsTr("Nickname (optional):")
		inputMethodHints: Qt.ImhPreferUppercase
		Layout.fillWidth: true
		inputField.onAccepted: joinGroupChat()
	}

	Connections {
		target: GroupChatController

		function onGroupChatJoined(accountJid, groupChatJid) {
			Kaidan.openChatPageRequested(accountJid, groupChatJid)
		}

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be joined%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function joinGroupChat() {
		if (groupChatJidField.valid) {
			GroupChatController.joinGroupChat(accountJid, groupChatJid, nicknameField.text)
		} else {
			groupChatJidField.forceActiveFocus()
		}
	}
}
