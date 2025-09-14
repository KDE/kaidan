// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts

import im.kaidan.kaidan

import "fields"

ConfirmationArea {
	id: root

	property Account account
	property alias groupChatNameField: groupChatNameField

	confirmationButton.text: qsTr("Create")
	confirmationButton.onClicked: {
		if (groupChatIdField.visible) {
			groupChatIdField.invalidHintMayBeShown = true
		}

		confirm()
	}
	loadingArea.description: qsTr("Creating group chat…")
	busy: account.groupChatController.busy

	Field {
		id: groupChatNameField
		labelText: qsTr("Group Name (optional)")
		placeholderText: qsTr("Example Group")
		inputMethodHints: Qt.ImhPreferUppercase	
		Layout.fillWidth: true
		inputField.onAccepted: confirm()
	}

	Controls.Switch {
		id: publicGroupChatCheckBox
		text: qsTr("Public")
		onToggled: {
			if (checked) {
				groupChatIdField.forceActiveFocus()
			} else {
				groupChatIdField.invalidHintMayBeShown = false
				groupChatNameField.forceActiveFocus()
			}
		}
	}

	CredentialsField {
		id: groupChatIdField
		labelText: qsTr("Group ID")
		// Set the group chat ID as the entered group chat name while replacing all whitespaces by hyphens.
		text: groupChatNameField.text.replace(/ /g, "-").toLowerCase()
		placeholderText: qsTr("example-group")
		inputMethodHints: Qt.ImhPreferLowercase
		invalidHintText: qsTr("Enter a valid group ID")
		valid: text
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		inputField.onAccepted: {
			invalidHintMayBeShown = true
			confirm()
		}
	}

	Controls.Label {
		text: "@"
		visible: groupChatServiceJidComboBox.visible
		textFormat: Text.StyledText
	}

	Controls.ComboBox {
		id: groupChatServiceJidComboBox
		model: root.account.groupChatController.groupChatServiceJids()
		visible: publicGroupChatCheckBox.checked && count > 1
		Layout.fillWidth: true
	}

	Field {
		id: nicknameField
		labelText: qsTr("Nickname (optional):")
		text: root.account.settings.displayName
		inputMethodHints: Qt.ImhPreferUppercase
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		inputField.onAccepted: confirm()
	}

	Connections {
		target: root.account.groupChatController

		function onGroupChatCreated(groupChatJid) {
			root.account.groupChatController.renameGroupChat(groupChatJid, groupChatNameField.text)
			root.account.groupChatController.joinGroupChat(groupChatJid, nicknameField.text)
		}

		function onPrivateGroupChatCreationFailed(serviceJid, errorMessage) {
			passiveNotification(qsTr("The group could not be created on %1%2").arg(serviceJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onPublicGroupChatCreationFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be created%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be joined%2").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function confirm() {
		if (groupChatIdField.visible && !groupChatIdField.valid) {
			groupChatIdField.forceActiveFocus()
			return
		}

		const serviceJid = groupChatServiceJidComboBox.textAt(groupChatServiceJidComboBox.currentIndex)

		if (publicGroupChatCheckBox.checked) {
			account.groupChatController.createPublicGroupChat(groupChatIdField.text + "@" + serviceJid)
		} else {
			account.groupChatController.createPrivateGroupChat(serviceJid)
		}

	}
}
