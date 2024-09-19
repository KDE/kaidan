// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a sheet listing the participants of a group chat in order to mention them.
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
				avatar.initialsMode: Kirigami.Avatar.InitialsMode.UseIcon
				avatar.color: Kirigami.Theme.textColor
				implicitWidth: largeButtonWidth
				hoverEnabled: true
				onClicked: GroupChatController.groupChatInviteeSelectionNeeded()
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
	}

	function search() {
		listView.model.setFilterFixedString(searchedText.substring(1, searchedText.length).toLowerCase())
		listView.currentIndex = 0
	}

	function selectCurrentIndex() {
		mentionParticipant(listView.model.data(listView.model.index(listView.currentIndex, 0), GroupChatUserModel.Role.Name))
	}

	function mentionParticipant(nickname) {
		textArea.remove(textArea.cursorPosition - searchedText.length + 1, textArea.cursorPosition)
		textArea.insert(textArea.cursorPosition, nickname + " ")
		close()
	}
}
