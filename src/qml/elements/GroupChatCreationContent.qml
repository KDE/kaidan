// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14

import im.kaidan.kaidan 1.0

import "fields"

ConfirmationArea {
	id: root

	required property string accountJid
	property alias groupChatNameField: groupChatNameField
	property alias groupChatName: groupChatNameField.text
	property alias groupChatIdField: groupChatIdField
	property alias nicknameField: nicknameField
	property alias nickname: nicknameField.text

	confirmationButton.text: qsTr("Create")
	confirmationButton.onClicked: createGroupChat()
	loadingArea.description: qsTr("Creating group chat…")

	Field {
		id: groupChatNameField
		labelText: qsTr("Group Name (optional)")
		placeholderText: qsTr("Example Group")
		inputMethodHints: Qt.ImhPreferUppercase	
		Layout.fillWidth: true
		inputField.onAccepted: createGroupChat()
	}

	Controls.CheckBox {
		id: publicGroupChatCheckBox
		text: qsTr("Public")
		onToggled: groupChatIdField.forceActiveFocus()
	}

	CredentialsField {
		id: groupChatIdField
		labelText: qsTr("Group ID")
		// Set the group chat ID as the entered group chat name while replacing all whitespaces by hyphens.
		text: groupChatNameField.text.replace(/ /g, "-").toLowerCase()
		placeholderText: qsTr("example-group")
		inputMethodHints: Qt.ImhPreferLowercase
		invalidHintText: qsTr("Enter a valid group ID")
		invalidHintMayBeShown: publicGroupChatCheckBox.checked
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		// Validate the entered group chat ID and handle an invalid one.
		onTextChanged: {
			if (text === "") {
				valid = false
			} else {
				valid = true
			}

			toggleHintForInvalidText()
		}
		inputField.onAccepted: createGroupChat()
	}

	Controls.Label {
		text: "@"
		visible: groupChatServiceJidComboBox.visible
		textFormat: Text.StyledText
	}

	Controls.ComboBox {
		id: groupChatServiceJidComboBox
		model: GroupChatController.groupChatServiceJids(root.accountJid)
		visible: publicGroupChatCheckBox.checked && count > 1
		Layout.fillWidth: true
	}

	Field {
		id: nicknameField
		labelText: qsTr("Nickname (optional):")
		inputMethodHints: Qt.ImhPreferUppercase
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		inputField.onAccepted: createGroupChat()
	}

	Connections {
		target: GroupChatController

		function onGroupChatCreated(accountJid, groupChatJid) {
			GroupChatController.joinGroupChat(accountJid, groupChatJid, root.nickname)
		}

		function onPrivateGroupChatCreationFailed(serviceJid, errorMessage) {
			root.busy = false
			passiveNotification(qsTr("The group could not be created on %1%2").arg(serviceJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onPublicGroupChatCreationFailed(groupChatJid, errorMessage) {
			root.busy = false
			passiveNotification(qsTr("The group %1 could not be created%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onGroupChatJoined(accountJid, groupChatJid) {
			GroupChatController.renameGroupChat(accountJid, groupChatJid, root.groupChatName)
			Kaidan.openChatPageRequested(accountJid, groupChatJid)
		}

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			root.busy = false
			passiveNotification(qsTr("The group %1 could not be joined%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function createGroupChat() {
		if (publicGroupChatCheckBox.checked && !groupChatIdField.valid) {
			groupChatIdField.toggleHintForInvalidText()
			groupChatIdField.forceActiveFocus()
			return
		}

		root.busy = true

		const serviceJid = groupChatServiceJidComboBox.textAt(groupChatServiceJidComboBox.currentIndex)

		if (publicGroupChatCheckBox.checked) {
			GroupChatController.createPublicGroupChat(accountJid, groupChatIdField.text + "@" + serviceJid)
		} else {
			GroupChatController.createPrivateGroupChat(accountJid, serviceJid)
		}

	}
}
