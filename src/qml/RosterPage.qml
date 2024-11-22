// SPDX-FileCopyrightText: 2016 Marzanna <MRZA-MRZA@users.noreply.github.com>
// SPDX-FileCopyrightText: 2016 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2019 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 caca hueto <cacahueto@olomono.de>
// SPDX-FileCopyrightText: 2020 Mathis Brüchert <mbblp@protonmail.ch>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

import "elements"

SearchBarPage {
	id: root
	listView: rosterListView
	rightAction: Kirigami.Action {
		text: qsTr("Filter")
		icon.name: "filter-symbolic"
		displayHint: Kirigami.DisplayHint.IconOnly
		onTriggered: openView(rosterFilteringDialog, rosterFilteringPage)
	}

	Component {
		id: rosterFilteringDialog

		Dialog {
			title: qsTr("Filter")
			onClosed: root.searchField.forceActiveFocus()

			RosterFilteringArea {
				rosterFilterProxyModel: filterModel
			}
		}
	}

	Component {
		id: rosterFilteringPage

		Kirigami.ScrollablePage {
			title: qsTr("Filter")
			background: Rectangle {
				color: Kirigami.Theme.alternateBackgroundColor
			}
			bottomPadding: 0

			RosterFilteringArea {
				rosterFilterProxyModel: filterModel
			}
		}
	}

	ListView {
		id: rosterListView

		QtObject {
			id: _previousMove

			property int oldIndex: -1
			property int newIndex: -1

			function reset() {
				_previousMove.oldIndex = -1
				_previousMove.newIndex = -1
			}
		}

		model: RosterFilterProxyModel {
			id: filterModel
			sourceModel: RosterModel
		}

		RosterListItemContextMenu {
			id: itemContextMenu
		}

		delegate: RosterListItem {
			highlighted: _previousMove.newIndex === model.index && _previousMove.oldIndex !== model.index
			width: rosterListView.width
			listView: rosterListView
			contextMenu: itemContextMenu
			accountJid: model ? model.accountJid : ""
			jid: model ? model.jid : ""
			name: model ? model.name : ""
			isGroupChat: model ? model.isGroupChat : false
			isPublicGroupChat: model ? model.isPublicGroupChat : false
			isDeletedGroupChat: model ? model.isDeletedGroupChat : false
			lastMessageDateTime: model ? model.lastMessageDateTime : ""
			lastMessage: model ? model.lastMessage : ""
			lastMessageIsDraft: model ? model.lastMessageIsDraft : false
			lastMessageIsOwn: model ? model.lastMessageIsOwn : false
			lastMessageGroupChatSenderName: model ? model.lastMessageGroupChatSenderName : ""
			unreadMessages: model ? model.unreadMessages : 0
			pinned: model ? model.pinned : false
			notificationRule: model ? model.notificationRule : false

			onClicked: {
				// Open the chatPage only if it is not yet open.
				// Emitting the signal is needed because there are slots in other places.
				if (!selected || !wideScreen) {
					Kaidan.openChatPageRequested(accountJid, jid)
				}
			}

			onMoveRequested: {
				_previousMove.oldIndex = oldIndex
				_previousMove.newIndex = newIndex
			}

			onDropRequested: {
				rosterListView.model.reorderPinnedItem(accountJid, jid, oldIndex, newIndex)
				_previousMove.reset()
			}
		}

		moveDisplaced: Transition {
			YAnimator {
				duration: Kirigami.Units.longDuration
				easing.type: Easing.InOutQuad
			}
		}

		Connections {
			target: Kaidan

			/**
			 * Opens the chat page for the chat JID currently set in the message model.
			 *
			 * @param accountJid JID of the account for that the chat page is opened
			 * @param chatJid JID of the chat for that the chat page is opened
			 */
			function onOpenChatPageRequested(accountJid, chatJid) {
				if (Kirigami.Settings.isMobile) {
					toggleSearchBar()
				} else {
					searchField.clear()
				}

				ChatController.setChat(accountJid, chatJid)

				closePagesExceptRosterPage()
				popLayersAboveLowest()
				pageStack.push(chatPage)
			}

			function onCloseChatPageRequested() {
				ChatController.resetChat()

				closePagesExceptRosterPage()
				resetChatView()
			}

			/**
			 * Closes all pages (especially the chat page) on the same layer except the roster page.
			 */
			function closePagesExceptRosterPage() {
				while (pageStack.depth > 1) {
					pageStack.pop()
				}
			}
		}
	}
}
