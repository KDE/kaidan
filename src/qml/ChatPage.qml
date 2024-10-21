// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2017 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2018 Ilya Bizyaev <bizyaev@zoho.com>
// SPDX-FileCopyrightText: 2019 Xavier <xavi@delape.net>
// SPDX-FileCopyrightText: 2019 Robert Maerkisch <zatrox@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2019 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2022 Mathis Brüchert <mbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Bhavy Airi <airiraghav@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import QtMultimedia 5.15 as Multimedia
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

import "elements"
import "details"

ChatPageBase {
	id: root

	DropArea {
		anchors.fill: parent
		onDropped: (drop) => {
			for (const url of drop.urls) {
				sendMediaSheet.addFile(url)
			}
			sendMediaSheet.ensureOpen()
		}
	}

	Shortcut {
		sequence: "Ctrl+Shift+V"
		context: Qt.WindowShortcut
		onActivated: {
			let imageUrl = Utils.pasteImage();
			// check if there was an image to be pasted from the clipboard
			if (imageUrl.toString().length > 0) {
				sendMediaSheet.addFile(imageUrl)
				sendMediaSheet.ensureOpen()
			}
		}
	}

	property alias searchBar: searchBar
	property alias sendMediaSheet: sendMediaSheet
	property alias newMediaSheet: newMediaSheet
	property alias messageReactionEmojiPicker: messageReactionEmojiPicker
	property alias messageReactionDetailsDialog: messageReactionDetailsDialog
	property alias messageListView: messageListView

	property ChatPageSendingPane sendingPane
	property ChatInfo globalChatDate
	readonly property bool cameraAvailable: Multimedia.QtMultimedia.availableCameras.length > 0
	property bool viewPositioned: false

	titleDelegate: Controls.ToolButton {
		visible: !Kirigami.Settings.isMobile

		contentItem: RowLayout {
			// weirdly having an id here, although unused, fixes the layout
			id: layout

			Avatar {
				Layout.leftMargin: Kirigami.Units.largeSpacing
				Layout.preferredHeight: parent.height
				Layout.preferredWidth: parent.height
				jid: ChatController.chatJid
				name: ChatController.rosterItem.displayName
			}
			Kirigami.Heading {
				Layout.fillWidth: true
				Layout.leftMargin: Kirigami.Units.largeSpacing
				Layout.rightMargin: Kirigami.Units.largeSpacing
				text: ChatController.rosterItem.displayName
			}
		}

		onClicked: {
			if (ChatController.accountJid === ChatController.chatJid) {
				openOverlay(notesChatDetailsDialog)
			} else if (ChatController.rosterItem.isGroupChat) {
				openOverlay(groupChatDetailsDialog)
			} else {
				openOverlay(contactDetailsDialog)
			}
		}
	}
	keyboardNavigationEnabled: true
	contextualActions: [
		Kirigami.Action {
			visible: Kirigami.Settings.isMobile
			icon.name: "avatar-default-symbolic"
			text: qsTr("Details…")
			onTriggered: {
				if (ChatController.accountJid === ChatController.chatJid) {
					openPage(notesChatDetailsPage)
				} else if (ChatController.rosterItem.isGroupChat) {
					openPage(groupChatDetailsPage)
				} else {
					openPage(contactDetailsPage)
				}
			}
		},
		// Action to toggle the message search bar
		Kirigami.Action {
			id: searchAction
			text: qsTr("Search")
			icon.name: "system-search-symbolic"
			displayHint: Kirigami.DisplayHint.IconOnly
			onTriggered: {
				if (searchBar.active)
					searchBar.close()
				else
					searchBar.open()
			}
		},
		Kirigami.Action {
			visible: !sendingPane.composition.isSpoiler
			icon.name: "password-show-off"
			text: qsTr("Add hidden message part")
			displayHint: Kirigami.DisplayHint.IconOnly
			onTriggered: sendingPane.prepareSpoiler()
		}
	]

	// Message search bar
	header: ChatPageSearchView {
		id: searchBar
		messageListView: root.messageListView
	}

	Component {
		id: notesChatDetailsDialog

		NotesChatDetailsDialog {}
	}

	Component {
		id: notesChatDetailsPage

		NotesChatDetailsPage {}
	}

	Component {
		id: notesChatDetailsKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			accountJid: ChatController.accountJid
			Component.onDestruction: openView(notesChatDetailsDialog, notesChatDetailsPage)
		}
	}

	Component {
		id: contactDetailsDialog

		ContactDetailsDialog {}
	}

	Component {
		id: contactDetailsPage

		ContactDetailsPage {}
	}

	Component {
		id: contactDetailsAccountKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			accountJid: ChatController.accountJid
			Component.onDestruction: openView(contactDetailsDialog, contactDetailsPage)
		}
	}

	Component {
		id: contactDetailsKeyAuthenticationPage

		ContactKeyAuthenticationPage {
			accountJid: ChatController.accountJid
			jid: ChatController.chatJid
			Component.onDestruction: openView(contactDetailsDialog, contactDetailsPage)
		}
	}

	Component {
		id: groupChatDetailsDialog

		GroupChatDetailsDialog {}
	}

	Component {
		id: groupChatDetailsPage

		GroupChatDetailsPage {}
	}

	Component {
		id: groupChatDetailsAccountKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			accountJid: ChatController.accountJid
			Component.onDestruction: openView(groupChatDetailsDialog, groupChatDetailsPage)
		}
	}

	Component {
		id: groupChatDetailsKeyAuthenticationPage

		GroupChatUserKeyAuthenticationPage {
			accountJid: ChatController.accountJid
			Component.onDestruction: openView(groupChatDetailsDialog, groupChatDetailsPage).openKeyAuthenticationUserListView()
		}
	}

	SendMediaSheet {
		id: sendMediaSheet
		composition: sendingPane.composition
		chatPage: parent
	}

	NewMediaSheet {
		id: newMediaSheet
		composition: sendingPane.composition
	}

	MessageReactionEmojiPicker {
		id: messageReactionEmojiPicker
	}

	MessageReactionDetailsDialog {
		id: messageReactionDetailsDialog
		accountJid: ChatController.accountJid
		chatJid: ChatController.chatJid
	}

	// View containing the messages
	ListView {
		id: messageListView
		verticalLayoutDirection: ListView.BottomToTop
		spacing: 0
		footerPositioning: ListView.OverlayFooter
		section.property: "nextDate"
		section.delegate: ColumnLayout {
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 0

			Item {
				height: Kirigami.Units.smallSpacing * 3
			}

			// placeholder for the hidden chatDate
			Item {
				height: chatDate.height
				visible: !chatDate.visible
			}

			ChatInfo {
				id: chatDate
				text: section
				// Hide the date if the section label would display the same date as globalChatDate.
				visible: root.globalChatDate && text !== root.globalChatDate.text
			}

			Item {
				height: Kirigami.Units.smallSpacing
			}
		}

		// Highlighting of the message containing a searched string.
		highlight: Component {
			Rectangle {
				id: highlightBar
				property int formerCurrentIndex

				height: messageListView.currentItem ? messageListView.currentItem.implicitHeight : 0
				width: messageListView.currentItem ? messageListView.currentItem.width + Kirigami.Units.smallSpacing * 2 : 0
				color: Kirigami.Theme.hoverColor

				// This is used to make the highlight bar a little bit bigger than the highlighted message.
				// It works only together with "messageListView.highlightFollowsCurrentItem: false".
				y: messageListView.currentItem ? messageListView.currentItem.y : 0
				x: messageListView.currentItem ? messageListView.currentItem.x : 0

				Connections {
					target: messageListView

					function onCurrentIndexChanged() {
						// Delay the destruction of the highlight bar in order to fade out the item.
						if (highlightBar.formerCurrentIndex === -1) {
							if (messageListView.currentIndex !== -1) {
								destructionOpacityAnimator.start()
							}
						} else if (messageListView.currentIndex === -1) {
							const formerCurrentIndex = highlightBar.formerCurrentIndex
							highlightBar.formerCurrentIndex = -1
							messageListView.currentIndex = formerCurrentIndex
						} else {
							highlightBar.formerCurrentIndex = messageListView.currentIndex
						}
					}
				}

				Behavior on y {
					SmoothedAnimation {
						velocity: 1000
						duration: 500
					}
				}

				Behavior on height {
					SmoothedAnimation {
						velocity: 1000
						duration: 500
					}
				}

				OpacityAnimator on opacity {
					from: 0
					to: 0.2
					duration: Kirigami.Units.shortDuration
				}

				OpacityAnimator on opacity {
					id: destructionOpacityAnimator
					from: 0.2
					to: 0
					duration: Kirigami.Units.shortDuration
					running: false
					onFinished: messageListView.currentIndex = -1
				}
			}
		}
		// This is used to make the highlight bar a little bit bigger than the highlighted message.
		highlightFollowsCurrentItem: false

		// Initially highlighted value
		currentIndex: -1

		// Connect to the database,
		model: MessageModel

		visibleArea.onYPositionChanged: handleMessageRead()
		onActiveFocusChanged: {
			// This makes it possible on desktop devices to directly enter a message after opening
			// the chat page.
			// The workaround is needed because messageListView's focus is automatically forced
			// after creation even when forcing sendingPane's focus within its
			// Component.onCompleted.
			if (activeFocus) {
				sendingPane.forceActiveFocus()
			}
		}

		Connections {
			target: MessageModel

			function onMessageFetchingFinished() {
				// Skip the case when messages are fetched after the initial fetching because this
				// function positioned the view at firstUnreadContactMessageIndex and that is close
				// to the end of the loaded messages.
				if (!root.viewPositioned) {
					let unreadMessageCount = ChatController.rosterItem.unreadMessageCount

					if (unreadMessageCount) {
						let firstUnreadContactMessageIndex = MessageModel.firstUnreadContactMessageIndex()

						if (firstUnreadContactMessageIndex > 0) {
							messageListView.positionViewAtIndex(firstUnreadContactMessageIndex, ListView.End)
						}

						root.viewPositioned = true

						// Trigger sending read markers manually as the view is ready.
						messageListView.handleMessageReadOnPositionedView()
					} else {
						root.viewPositioned = true
					}
				}
			}
		}

		Connections {
			target: Qt.application

			function onStateChanged(state) {
				// Send a read marker once the application becomes active if a message has been received while the application was not active.
				if (state === Qt.ApplicationActive) {
					messageListView.handleMessageRead()
				}
			}
		}

		/**
		 * Sends a read marker for the latest visible/read message if the view is positioned.
		 */
		function handleMessageRead() {
			if (root.viewPositioned) {
				handleMessageReadOnPositionedView()
			}
		}

		/**
		 * Sends a read marker for the latest visible/read message.
		 */
		function handleMessageReadOnPositionedView() {
			MessageModel.handleMessageRead(indexAt(0, (contentY + height + 15)) + 1)
		}

		delegate: ChatMessage {
			messageListView: root.messageListView
			sendingPane: root.sendingPane
			reactionEmojiPicker: root.messageReactionEmojiPicker
			reactionDetailsDialog: root.messageReactionDetailsDialog
			modelIndex: index
			msgId: model.id
			senderJid: model.senderJid
			groupChatSenderId: model.groupChatSenderId
			senderName: model.senderName
			chatName: ChatController.rosterItem.displayName
			encryption: model.encryption
			trustLevel: model.trustLevel
			isGroupChatMessage: ChatController.rosterItem.isGroupChat
			isOwn: model.isOwn
			replyToJid: model.replyToJid
			replyToGroupChatParticipantId: model.replyToGroupChatParticipantId
			replyToName: model.replyToName
			replyId: model.replyId
			replyQuote: model.replyQuote
			messageBody: model.body
			date: model.date
			time: model.time
			deliveryState: model.deliveryState
			deliveryStateName: model.deliveryStateName
			deliveryStateIcon: model.deliveryStateIcon
			isLastReadOwnMessage: model.isLastReadOwnMessage
			isLastReadContactMessage: model.isLastReadContactMessage
			edited: model.isEdited
			isSpoiler: model.isSpoiler
			spoilerHint: model.spoilerHint
			errorText: model.errorText
			files: model.files
			displayedReactions: model.displayedReactions
			detailedReactions: model.detailedReactions
			ownReactionsFailed: model.ownReactionsFailed
			groupChatInvitationJid: model.groupChatInvitationJid
		}

		// Everything is upside down, looks like a footer
		header: ColumnLayout {
			visible: ChatController.accountJid !== ChatController.chatJid
			anchors.left: parent.left
			anchors.right: parent.right
			height: Kirigami.Units.largeSpacing * 3

			// chat state
			Controls.Label {
				text: Utils.chatStateDescription(ChatController.rosterItem.displayName, ChatController.chatState)
				elide: Qt.ElideMiddle
				Layout.alignment: Qt.AlignCenter
				Layout.maximumWidth: parent.width - Kirigami.Units.largeSpacing * 4
			}
		}

		footer: ColumnLayout {
			z: 2
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 0

			// date of the top-most (first) visible message shown at the top of the ChatPage
			ChatInfo {
				text: {
					// "root.viewPositioned" is checked in order to update/display the text when the
					// ChatPage is opened or messages are fetched.
					// "messageListView.contentHeight" is checked in order to update the margin when
					// messages are added or updated (resulting in a larger height).
					if (root.viewPositioned && messageListView.contentHeight && messageListView.count > 0) {
						// Search until a message is found if there is a section label instead of a
						// message at the top of the visible/content area.
						// "i" is used as an offset.
						for (let i = 0; i < messageListView.height; i++) {
							const firstVisibleMessage = messageListView.itemAt(0, messageListView.contentY + i)

							if (firstVisibleMessage) {
								return firstVisibleMessage.date
							}
						}

						return ""
					}

					return ""
				}
				visible: text.length
				Layout.topMargin: {
					const minimalMargin = Kirigami.Units.smallSpacing * 3
					const oldestMessage = messageListView.itemAtIndex(messageListView.count - 1)

					// "root.viewPositioned" is checked in order to update the margin when the
					// ChatPage is opened or messages are fetched.
					// "messageListView.contentHeight" is checked in order to update the margin when
					// messages are added or updated (resulting in a larger height).
					// "messageListView.visibleArea.yPosition" is checked in order to update the
					// margin when the ListView is scrolled.
					// "oldestMessage" must be at the end of the condition (for unclear reasons).
					// Otherwise, adding a message would always result in using "minimalMargin".
					if (root.viewPositioned && messageListView.contentHeight && messageListView.visibleArea.yPosition >= 0 && oldestMessage) {
						const spaceBetweenTopAndOldestMessage = messageListView.height + oldestMessage.y
						return Math.max(minimalMargin, spaceBetweenTopAndOldestMessage - Kirigami.Units.smallSpacing * 10 - messageListView.headerItem.height)
					}

					return minimalMargin
				}
				Component.onCompleted: root.globalChatDate = this
			}

			Item {
				height: Kirigami.Units.smallSpacing
			}
		}

		// button for jumping to the latest message
		Controls.RoundButton {
			visible: width > 0
			width: parent.atYEnd ? 0 : 50
			height: parent.atYEnd ? 0 : 50
			anchors.right: parent.right
			anchors.bottom: parent.bottom
			anchors.bottomMargin: 7
			anchors.rightMargin: {
				if (root.flickable.Controls.ScrollBar.vertical) {
					return Kirigami.Settings.isMobile
						? root.flickable.Controls.ScrollBar.vertical.implicitWidth + 15
						: root.flickable.Controls.ScrollBar.vertical.implicitWidth + 5
				}

				return Kirigami.Settings.isMobile ? 15 : 5
			}
			icon.name: "go-down-symbolic"
			onClicked: parent.positionViewAtIndex(0, ListView.Center)

			Behavior on width {
				SmoothedAnimation {}
			}

			Behavior on height {
				SmoothedAnimation {}
			}

			MessageCounter {
				id: unreadMessageCounter
				count: ChatController.rosterItem.unreadMessageCount
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.top
				anchors.verticalCenterOffset: -2
			}
		}

		Timer {
			id: resetCurrentIndexTimer
			interval: Kirigami.Units.veryLongDuration * 4
			onTriggered: messageListView.currentIndex = -1
		}

		function highlightShortly(index) {
			currentIndex = index
			resetCurrentIndexTimer.restart()
		}
	}

	footer:	ListView {
		id: chatHintListView
		height: contentHeight
		verticalLayoutDirection: ListView.BottomToTop
		interactive: false
		headerPositioning: ListView.OverlayHeader
		model: ChatHintModel
		delegate: ChatHintArea {
			id: chatHintArea
			index: model.index
			text: model.text
			buttons: model.buttons
			loading: model.loading
			loadingDescription: model.loadingDescription
			anchors.bottom: root.sendingPane.top

			Connections {
				target: chatHintListView

				function onCountChanged() {
					if (chatHintListView.count > 0) {
						chatHintArea.enabled = chatHintArea.index === chatHintListView.count - 1
					} else {
						chatHintArea.enabled = false
					}
				}
			}
		}
		header: ChatPageSendingPane {
			chatPage: root
			width: root.width
			height: ChatController.rosterItem.isDeletedGroupChat ? 0 : undefined
			Component.onCompleted: root.sendingPane = this
		}
		onCountChanged: {
			// Make it possible to directly enter a message after chatHintListView is focused because of changes in its model.
			if (root.sendingPane) {
				root.sendingPane.forceActiveFocus()
			}
		}
	}

	Connections {
		target: Kaidan.groupChatController

		function onGroupChatInviteeSelectionNeeded() {
			openView(groupChatDetailsDialog, groupChatDetailsPage).openContactListView()
		}
	}
}
