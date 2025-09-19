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
Dialog {
	id: root

	property Account account
	property string chatJid
	property Controls.TextArea textArea
	property string searchedText
	property alias listView: listView

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	// Set a negative inset to fix the rounded corner of the dialog above the scroll bar if visible.
	topInset: contentItem.Controls.ScrollBar.vertical.visible ? - Kirigami.Units.cornerRadius : 0
	modal: false
	header: null

	ListView {
		id: listView
		model: GroupChatUserFilterModel {
			sourceModel: GroupChatUserModel {
				accountJid: root.account.settings.jid
				chatJid: root.chatJid
			}
		}
		header: count ? null : contactInvitationHint
		delegate: ContactDelegate {
			account: root.account
			jid: model.jid
			name: model.name
			avatar.accountAvatar.visible: false
			width: ListView.view.width
			hoverEnabled: true
			checked: ListView.isCurrentItem
			onClicked: mentionParticipant(name)
		}

		Component {
			id: contactInvitationHint

			ContactDelegate {
				name: qsTr("Invite contacts to this group…")
				textItem.font.italic: true
				avatar.iconSource: "resource-group-new"
				avatar.initialsMode: Components.Avatar.InitialsMode.UseIcon
				avatar.color: Kirigami.Theme.textColor
				implicitWidth: largeButtonWidth
				implicitHeight: height
				hoverEnabled: true
				checked: true
				onClicked: {
					root.close()
					root.textArea.remove(root.textArea.cursorPosition - searchedText.length, textArea.cursorPosition)
					root.account.groupChatController.groupChatInviteeSelectionNeeded()
				}
			}
		}
	}

	onClosed: {
		searchedText = ""
		listView.currentIndex = -1
	}

	function prepareSearch(currentCharacter) {
		searchedText += currentCharacter
		listView.currentIndex = 0
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
