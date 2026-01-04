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

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "elements"

SearchBarPage {
	id: root

	property ChatPage activeChatPage

	listView: rosterListView
	actions: [
		Kirigami.Action {
			text: qsTr("Filter")
			icon.name: "filter-symbolic"
			displayHint: Kirigami.DisplayHint.IconOnly
			onTriggered: openView(rosterFilterDialog, rosterFilterPage)
		},

		Kirigami.Action {
			id: pinAction
			text: qsTr("Pin & Move")
			icon.name: "non-starred-symbolic"
			displayHint: Kirigami.DisplayHint.IconOnly
			checkable: true
			onToggled: root.searchField.forceActiveFocus()
		}
	]

	ListView {
		id: rosterListView
		model: RosterFilterModel {
			id: filterModel
			sourceModel: RosterModel
		}
		delegate: RosterItemDelegate {
			property bool active: root.activeChatPage && root.activeChatPage.chatController.account.settings.jid === accountJid && root.activeChatPage.chatController.jid === jid

			highlighted: pinned && _previousMove.newIndex === model.index && _previousMove.oldIndex !== model.index
			checked: !Kirigami.Settings.isMobile && active
			width: rosterListView.width
			listView: rosterListView
			accountJid: model.account.settings.jid
			accountName: model.account.settings.displayName
			jid: model.jid
			name: model.name
			isNotesChat: model.isNotesChat
			isProviderChat: model.isProviderChat
			isGroupChat: model.isGroupChat
			isPublicGroupChat: model.isPublicGroupChat
			isDeletedGroupChat: model.isDeletedGroupChat
			lastMessageDateTime: model.lastMessageDateTime
			lastMessage: model.lastMessage
			lastMessageIsDraft: model.lastMessageIsDraft
			lastMessageIsOwn: model.lastMessageIsOwn
			lastMessageGroupChatSenderName: model.lastMessageGroupChatSenderName
			unreadMessageCount: model.unreadMessageCount
			markedMessageCount: model.markedMessageCount
			pinModeActive: pinAction.checked
			pinned: model.pinned
			effectiveNotificationRule: model.effectiveNotificationRule
			onClicked: {
				// Open the chatPage.
				if (active) {
					if (!pageStack.wideMode) {
						pageStack.goForward()
					}
				} else {
					// Emitting the signal is needed because there are slots in other places.
					MainController.openChatPageRequested(account.settings.jid, jid)
				}
			}
			onMoveRequested: (oldIndex, newIndex) => {
				_previousMove.oldIndex = oldIndex
				_previousMove.newIndex = newIndex
			}
			onDropRequested: (oldIndex, newIndex) => {
				rosterListView.model.reorderPinnedItem(oldIndex, newIndex)
				_previousMove.reset()
			}
		}
		moveDisplaced: Transition {
			YAnimator {
				duration: Kirigami.Units.longDuration
				easing.type: Easing.InOutQuad
			}
		}

		QtObject {
			id: _previousMove

			property int oldIndex: -1
			property int newIndex: -1

			function reset() {
				_previousMove.oldIndex = -1
				_previousMove.newIndex = -1
			}
		}

		Connections {
			target: MainController

			/**
			 * Opens the chat page for the chat JID currently set in the message model.
			 *
			 * @param accountJid JID of the account for that the chat page is opened
			 * @param chatJid JID of the chat for that the chat page is opened
			 */
			function onOpenChatPageRequested(accountJid, chatJid) {
				globalDrawer.close()

				if (Kirigami.Settings.isMobile) {
					root.toggleSearchBar()
				} else {
					root.searchField.clear()
				}

				if (!root.activeChatPage) {
					closePagesExceptRosterPage()
					popLayersAboveLowest()
					root.activeChatPage = pageStack.push(chatPage)
				}

				if ((root.activeChatPage.chatController.account && root.activeChatPage.chatController.account.settings.jid !== accountJid) || chatJid !== root.activeChatPage.chatController.jid) {
					root.activeChatPage.initialize(accountJid, chatJid)
				}

				if (!pageStack.wideMode) {
					pageStack.goForward()
				}

				root.activeChatPage.forceActiveFocus()
			}

			function onCloseChatPageRequested() {
				root.activeChatPage = null

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

		Connections {
			target: RosterModel
			enabled: root.activeChatPage

			function onItemRemoved(accountJid, chatJid) {
				if (accountJid === root.activeChatPage.chatController.account.settings.jid && chatJid === root.activeChatPage.chatController.jid) {
					MainController.closeChatPageRequested()
				}
			}

			function onItemsRemoved(accountJid) {
				if (accountJid === root.activeChatPage.chatController.account.settings.jid) {
					MainController.closeChatPageRequested()
				}
			}
		}

		Connections {
			target: root.activeChatPage ? root.activeChatPage.chatController : null

			property var previouslySelectedAccountJids

			function onMessageBodyToForwardChanged() {
				if (root.activeChatPage.chatController.messageBodyToForward) {
					// "slice()" is needed to avoid a property binding.
					previouslySelectedAccountJids = filterModel.selectedAccountJids.slice()
					filterModel.selectedAccountJids = [root.activeChatPage.chatController.account.settings.jid]
				} else {
					filterModel.selectedAccountJids = previouslySelectedAccountJids
				}
			}
		}
	}

	Component {
		id: rosterFilterDialog

		Dialog {
			title: qsTr("Filter")
			onClosed: root.searchField.forceActiveFocus()

			RosterFilterArea {
				rosterFilterModel: filterModel
			}
		}
	}

	Component {
		id: rosterFilterPage

		Kirigami.ScrollablePage {
			title: qsTr("Filter")
			background: Rectangle {
				color: Kirigami.Theme.alternateBackgroundColor
			}
			bottomPadding: 0

			RosterFilterArea {
				rosterFilterModel: filterModel
			}
		}
	}

	Component {
		id: chatPage

		ChatPage {}
	}
}
