/*
 *  Kaidan - A user-friendly XMPP client for every device!
 *
 *  Copyright (C) 2016-2023 Kaidan developers and contributors
 *  (see the LICENSE file for a full list of copyright authors)
 *
 *  Kaidan is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  In addition, as a special exception, the author of Kaidan gives
 *  permission to link the code of its release with the OpenSSL
 *  project's "OpenSSL" library (or with modified versions of it that
 *  use the same license as the "OpenSSL" library), and distribute the
 *  linked executables. You must obey the GNU General Public License in
 *  all respects for all of the code used other than "OpenSSL". If you
 *  modify this file, you may extend this exception to your version of
 *  the file, but you are not obligated to do so.  If you do not wish to
 *  do so, delete this exception statement from your version.
 *
 *  Kaidan is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kaidan.  If not, see <http://www.gnu.org/licenses/>.
 */

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

	RosterAddContactSheet {
		id: addContactSheet
		jid: ""
	}

	mainAction: Kirigami.Action {
		text: qsTr("Add new contact")
		icon.name: "contact-new-symbolic"
		onTriggered: {
			if (addContactSheet.sheetOpen)
				addContactSheet.close()
			else
				addContactSheet.open()
		}
	}

	rightAction: Kirigami.Action {
		id: searchAction
		text: qsTr("Search contacts")
		checkable: true
		icon.name: "system-search-symbolic"
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

			function onXmppUriReceived(uri) {
				// 'xmpp:' has length of 5
				addContactSheet.jid = uri.substr(5)
				addContactSheet.open()
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
		for (var i = 0; i < pageStack.items.length; ++i) {
			var page = pageStack.items[i];

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
