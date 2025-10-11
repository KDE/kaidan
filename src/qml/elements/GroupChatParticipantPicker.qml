// SPDX-FileCopyrightText: 2022 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a dialog listing the participants of a group chat in order to mention them.
 */
Dialog {
	id: root

	property Account account
	property string chatJid
	property Controls.TextArea textArea
	readonly property alias searchedText: root._searchedText
	property string _searchedText
	property alias listView: listView

	topPadding: 0
	leftPadding: 0
	bottomPadding: 0
	rightPadding: 0
	// Set a negative inset to fix the rounded corner of the dialog above the scroll bar if visible.
	topInset: contentItem.Controls.ScrollBar.vertical.visible ? - Kirigami.Units.cornerRadius : 0
	modal: false
	header: null
	onOpened: textArea.forceActiveFocus()

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
			jid: model.jid
			name: model.name
			width: ListView.view.width
			hoverEnabled: true
			checked: ListView.isCurrentItem
			onClicked: mentionParticipant(name)
		}

		Component {
			id: contactInvitationHint

			ContactInvitationHintDelegate {
				onClicked: {
					root.close()
					root.textArea.remove(root.textArea.cursorPosition - root.searchedText.length, textArea.cursorPosition)
					root.account.groupChatController.groupChatInviteeSelectionNeeded()
				}
			}
		}
	}

	function search(text) {
		_searchedText = text
		listView.model.setFilterFixedString(searchedText.toLowerCase())
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
		textArea.remove(textArea.cursorPosition - searchedText.length, textArea.cursorPosition)
		textArea.mentionParticipant(nickname)
		close()
	}
}
