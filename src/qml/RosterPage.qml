// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami
import im.kaidan.kaidan 1.0
import "elements"

Kirigami.ScrollablePage {
	id: root
	title: {
		Kaidan.connectionState === Enums.StateConnecting ? qsTr("Connecting…") :
		Kaidan.connectionState === Enums.StateDisconnected ? qsTr("Offline") :
		qsTr("Contacts")
	}
	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0

	mainAction: Kirigami.Action {
		id: searchAction
		text: qsTr("Search contacts")
		checkable: true
		icon.name: "system-search-symbolic"
		displayHint: Kirigami.DisplayHint.IconOnly
		onTriggered: {
			if (checked) {
				searchField.forceActiveFocus()
				searchField.selectAll()
			}
		}
		shortcut: "Ctrl+F"
	}

	header: Item {
		height: searchField.visible ? searchField.height : 0
		clip: true

		Behavior on height {
			SmoothedAnimation {
				velocity: 200
			}
		}

		Kirigami.SearchField {
			id: searchField
			focusSequence: ""
			width: parent.width
			height: Kirigami.Units.gridUnit * 2
			visible: searchAction.checked
			onVisibleChanged: text = ""
			onTextChanged: filterModel.setFilterFixedString(text.toLowerCase())
		}
	}

	ListView {
		id: rosterListView

		width: root.width
		model: RosterFilterProxyModel {
			id: filterModel
			sourceModel: RosterModel
		}

		RosterListItemContextMenu {
			id: itemContextMenu
		}

		Component {
			id: delegateComponent

			RosterListItem {
				listView: rosterListView
				contextMenu: itemContextMenu
				accountJid: AccountManager.jid
				jid: model ? model.jid : ""
				name: model ? (model.name ? model.name : model.jid) : ""
				lastMessage: model ? model.lastMessage : ""
				lastMessageIsDraft: model ? model.draftId : false
				unreadMessages: model ? model.unreadMessages : 0
				pinned: model ? model.pinned : false
				notificationsMuted: model ? model.notificationsMuted : false

				onClicked: {
					// Open the chatPage only if it is not yet open.
					if (!isSelected || !wideScreen) {
						openChatPage(accountJid, jid)
					}
				}
			}
		}

		// TODO: Remove the DelegateRecycler if possible
		// Without the DelegateRecycler, the reordering of pinned items does not work.
		// But it is not clear why the DelegateRecycler is needed for that purpose because the
		// ListView would manage the recycling by itself if enabled/needed.
		delegate: Kirigami.DelegateRecycler {
			width: rosterListView.width
			sourceComponent: delegateComponent
		}

		moveDisplaced: Transition {
			YAnimator {
				duration: Kirigami.Units.longDuration
				easing.type: Easing.InOutQuad
			}
		}

		Connections {
			target: Kaidan

			function onOpenChatPageRequested(accountJid, chatJid) {
				openChatPage(accountJid, chatJid)
			}
		}
	}

	/**
	 * Opens the chat page for the chat JID currently set in the message model.
	 *
	 * @param accountJid JID of the account for that the chat page is opened
	 * @param chatJid JID of the chat for that the chat page is opened
	 */
	function openChatPage(accountJid, chatJid) {
		for (let i = 0; i < pageStack.items.length; ++i) {
			let page = pageStack.items[i];

			if (page instanceof ChatPage) {
				page.saveDraft();
			}
		}
		MessageModel.setCurrentChat(accountJid, chatJid)
		searchAction.checked = false

		// Close all pages (especially the chat page) except the roster page.
		while (pageStack.depth > 1)
			pageStack.pop()

		popLayersAboveLowest()
		pageStack.push(chatPage)
	}
}
