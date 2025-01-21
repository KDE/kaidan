// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.components as Components

import im.kaidan.kaidan

/**
 * This is a dialog listing the participants of a group chat in order to mention them.
 */
Kirigami.Dialog {
	id: root

	property string accountJid
	property string chatJid
	property Controls.TextArea textArea
	property string searchedText
	property alias listView: listView

	modal: false
	header: Item {}
	footer: Item {}

	ListView {
		id: listView
		implicitWidth: largeButtonWidth
		implicitHeight: contentHeight
		model: GroupChatUserFilterModel {
			sourceModel: GroupChatUserModel {
				accountJid: root.accountJid
				chatJid: root.chatJid
			}
		}
		header: count ? null : contactInvitationHint
		delegate: GroupChatUserItem {
			accountJid: root.accountJid
			jid: model.jid
			name: model.name
			width: ListView.view.width
			hoverEnabled: true
			selected: ListView.isCurrentItem
			onClicked: mentionParticipant(name)
		}

		Component {
			id: contactInvitationHint

			GroupChatUserItem {
				name: qsTr("Invite contacts to this group…")
				userText.font.italic: true
				avatar.iconSource: "resource-group-new"
				avatar.initialsMode: Components.Avatar.InitialsMode.UseIcon
				avatar.color: Kirigami.Theme.textColor
				implicitWidth: largeButtonWidth
				hoverEnabled: true
				onClicked: {
					root.close()
					root.textArea.remove(root.textArea.cursorPosition - searchedText.length, textArea.cursorPosition)
					GroupChatController.groupChatInviteeSelectionNeeded()
				}
			}
		}
	}

	onClosed: {
		searchedText = ""
		listView.currentIndex = -1
	}

	function toggle() {
		if (visible) {
			close()
		} else {
			open()
		}
	}

	function openForSearch(currentCharacter) {
		searchedText += currentCharacter
		listView.currentIndex = 0
		open()
		messageArea.forceActiveFocus()
	}

	function search() {
		listView.model.setFilterFixedString(searchedText.substring(1, searchedText.length).toLowerCase())
		listView.currentIndex = 0
	}

	function selectCurrentIndex() {
		if (listView.headerItem) {
			listView.headerItem.clicked()
		} else {
			mentionParticipant(listView.model.data(listView.model.index(listView.currentIndex, 0), GroupChatUserModel.Role.Name))
		}
	}

	function mentionParticipant(nickname) {
		textArea.remove(textArea.cursorPosition - searchedText.length + 1, textArea.cursorPosition)
		textArea.insert(textArea.cursorPosition, nickname + Utils.groupChatUserMentionSeparator)
		close()
	}
}
