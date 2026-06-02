// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

import "fields"

ConfirmationArea {
	id: root

	property Account account
	property alias groupChatNameField: groupChatNameField

	confirmationButton.text: qsTr("Create")
	confirmationButton.onClicked: confirm()
	loadingArea.description: qsTr("Creating group chat…")
	busy: account.groupChatController.busy

	Field {
		id: groupChatNameField
		label: qsTr("Group name (optional)")
		placeholderText: qsTr("Example Group")
		inputMethodHints: Qt.ImhPreferUppercase	
		Layout.fillWidth: true
		onAccepted: confirm()
	}

	Controls.Switch {
		id: publicGroupChatCheckBox
		text: qsTr("Public")
		Layout.leftMargin: FormCard.FormCardUnits.horizontalPadding
		Layout.rightMargin: FormCard.FormCardUnits.horizontalPadding
		onToggled: {
			if (checked) {
				groupChatIdField.forceActiveFocus()
			} else {
				groupChatNameField.forceActiveFocus()
			}
		}
	}

	Field {
		id: groupChatIdField
		label: qsTr("Group ID")
		// Set the group chat ID as the entered group chat name while replacing all whitespaces by hyphens.
		text: groupChatNameField.text.replace(/ /g, "-").toLowerCase()
		placeholderText: qsTr("example-group")
		inputMethodHints: Qt.ImhPreferLowercase
		invalidHintText: qsTr("Enter a valid group ID")
		inputValidator.patterns: InputValidator.Pattern.NotEmpty
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		onAccepted: confirm()
	}

	Controls.Label {
		text: "@"
		visible: groupChatServiceJidComboBox.visible
		Layout.leftMargin: FormCard.FormCardUnits.horizontalPadding
		Layout.rightMargin: FormCard.FormCardUnits.horizontalPadding
	}

	FormCard.FormComboBoxDelegate {
		id: groupChatServiceJidComboBox
		text: qsTr("Service")
		model: root.account.groupChatController.groupChatServiceJids()
		currentIndex: 0
		visible: publicGroupChatCheckBox.checked && count > 1
	}

	Field {
		id: nicknameField
		label: qsTr("Nickname (optional)")
		text: root.account.settings.displayName
		inputMethodHints: Qt.ImhPreferUppercase
		visible: publicGroupChatCheckBox.checked
		Layout.fillWidth: true
		onAccepted: confirm()
	}

	Connections {
		target: root.account.groupChatController

		function onGroupChatCreated(groupChatJid) {
			root.account.groupChatController.renameGroupChat(groupChatJid, groupChatNameField.text)
			root.account.groupChatController.joinGroupChat(groupChatJid, nicknameField.text)
		}

		function onPrivateGroupChatCreationFailed(serviceJid, errorMessage) {
			passiveNotification(qsTr("The group could not be created on %1%2", "%1 is a service JID, %2 is either empty or ': ' followed by an error message").arg(serviceJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onPublicGroupChatCreationFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be created%2", "%1 is a group JID, %2 is either empty or ': ' followed by an error message").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}

		function onGroupChatJoiningFailed(groupChatJid, errorMessage) {
			passiveNotification(qsTr("The group %1 could not be joined%2", "%1 is a group JID, %2 is either empty or ': ' followed by an error message").arg(groupChatJid).arg(errorMessage ? ": " + errorMessage : ""))
		}
	}

	function confirm() {
		if (groupChatIdField.visible && !groupChatIdField.valid) {
			groupChatIdField.forceActiveFocus()
			return
		}

		const serviceJid = groupChatServiceJidComboBox.currentText

		if (publicGroupChatCheckBox.checked) {
			account.groupChatController.createPublicGroupChat(groupChatIdField.text + "@" + serviceJid)
		} else {
			account.groupChatController.createPrivateGroupChat(serviceJid)
		}

	}
}
