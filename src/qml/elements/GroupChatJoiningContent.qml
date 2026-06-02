// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
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
	confirmationButton.onClicked: confirm()
	loadingArea.description: qsTr("Joining group…")
	busy: account.groupChatController.busy

	JidField {
		id: groupChatJidField
		label: qsTr("Group address")
		placeholderText: qsTr("group@groups.example.org")
		invalidHintText: qsTr("Enter a valid group address")
		inputMethodHints: Qt.ImhEmailCharactersOnly | Qt.ImhPreferLowercase
		Layout.fillWidth: true
		onTextEdited: {
			const jidOfXmppUri = Utils.jid(text)

			if (jidOfXmppUri) {
				// If the user inserts an XMPP URI into jidField, set jidField.text to the URI's JID.
				text = jidOfXmppUri
			}
		}
		onAccepted: {
			if (valid) {
				nicknameField.forceActiveFocus()
			} else {
				forceActiveFocus()
			}
		}
	}

	Field {
		id: nicknameField
		label: qsTr("Nickname (optional)")
		text: root.account.settings.displayName
		inputMethodHints: Qt.ImhPreferUppercase
		Layout.fillWidth: true
		onAccepted: confirm()
	}

	Connections {
		target: root.account.groupChatController

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be joined%2", "%1 is a group JID, %2 is either empty or ': ' followed by an error message").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function confirm() {
		if (groupChatJidField.valid) {
			account.groupChatController.joinGroupChat(groupChatJid, nicknameField.text)
		} else {
			groupChatJidField.forceActiveFocus()
		}
	}
}
