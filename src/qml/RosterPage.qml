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
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "elements"

SearchBarPage {
	id: root

	property ChatPage activeChatPage

	searchField {
		listView: rosterListView
		onTextChanged: rosterListView.model.setFilterFixedString(searchField.text.toLowerCase())
		Keys.onEscapePressed: rosterListView.resetCurrentIndex()
	}
	toolbarItems: [
		ToolbarButton {
			Controls.ToolTip.text: qsTr("Filter")
			source: "filter-symbolic"
			onClicked: openView(rosterFilterDialog, rosterFilterPage)
		},

		ToolbarButton {
			id: pinButton
			Controls.ToolTip.text: qsTr("Pin & Move")
			source: checked ? "starred-symbolic" : "non-starred-symbolic"
			checkable: true
			onClicked: root.forceActiveFocus()
		}
	]

	ListView {
		id: rosterListView
		currentIndex: -1
		model: RosterFilterModel {
			id: filterModel
			sourceModel: RosterModel
		}
		delegate: RosterItemDelegate {
			property bool active: root.activeChatPage && root.activeChatPage.chatController.account.settings.jid === accountJid && root.activeChatPage.chatController.jid === jid

			highlighted: rosterListView.currentIndex === model.index || pinned && _previousMove.newIndex === model.index && _previousMove.oldIndex !== model.index
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
			pinModeActive: pinButton.checked
			pinned: model.pinned
			effectiveNotificationRule: model.effectiveNotificationRule
			onClicked: {
				rosterListView.resetCurrentIndex()

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
		onActiveFocusChanged: {
			// This makes it possible on desktop devices to directly search for a chat after opening
			// RosterPage.
			// The workaround is needed because rosterListView's focus is automatically forced
			// after creation even when forcing searchFields's focus within its
			// Component.onCompleted.
			if (!Kirigami.Settings.isMobile && activeFocus) {
				root.forceActiveFocus()
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
				root.searchField.clear()

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

		function resetCurrentIndex() {
			currentIndex = -1
		}
	}

	Component {
		id: rosterFilterDialog

		Dialog {
			title: qsTr("Filter")
			onClosed: root.forceActiveFocus()

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
			padding: 0

			RosterFilterArea {
				rosterFilterModel: filterModel
			}
		}
	}

	Component {
		id: chatPage

		ChatPage {}
	}

	function forceActiveFocus() {
		if (!Kirigami.Settings.isMobile) {
			searchField.forceActiveFocus()
		}
	}
}
