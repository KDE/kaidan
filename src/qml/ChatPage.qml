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

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

import "elements"
import "details"

SearchBarPage {
	id: root

	property ChatController chatController: ChatController {}
	property alias messageReactionEmojiPicker: messageReactionEmojiPicker
	property alias messageListView: messageListView
	property ChatPageSendingPane sendingPane
	property ChatInfo globalChatDate
	property bool viewPositioned: false
	// Color of the message bubbles on the right side
	readonly property color rightMessageBubbleColor: {
		const accentColor = Kirigami.Theme.highlightColor
		return Qt.tint(primaryBackgroundColor, Qt.rgba(accentColor.r, accentColor.g, accentColor.b, 0.1))
	}

	background: ImageBackground {}
	bottomPadding: Kirigami.Units.largeSpacing
	searchField {
		opacity: searchButton.checked ? 1 : 0
		visible: searchField.opacity
		autoAccept: false
		focusSequence: "Ctrl+Shift+F"
		listView: messageListView
		onVisibleChanged: {
			if (searchField.visible) {
				searchField.forceActiveFocus()
			} else {
				searchField.reset()
				root.forceActiveFocus()
			}
		}
		onAccepted: searchFromCurrentIndex(true)
		Keys.onUpPressed: searchFromCurrentIndex(true)
		Keys.onDownPressed: searchFromCurrentIndex(false)
		Keys.onEscapePressed: searchButton.clicked()
	}
	separatorVisible: true
	toolbarItems: [
		Controls.AbstractButton {
			id: avatarActionButton
			visible: !root.searchField.visible
			focusPolicy: Qt.NoFocus
			hoverEnabled: true
			contentItem: RowLayout {
				AccountRelatedAvatar {
					jid: root.chatController.jid
					name: root.chatController.account.settings.jid === root.chatController.jid ? root.chatController.account.settings.displayName : root.chatController.rosterItem.displayName
					isProviderChat: root.chatController.rosterItem.isProviderChat
					isGroupChat: root.chatController.rosterItem.isGroupChat
					accountAvatar {
						jid: root.chatController.account.settings.jid
						name: root.chatController.account.settings.displayName
					}
					accountAvatarBorder.color: Kirigami.Theme.backgroundColor
					opacity: avatarActionButton.hovered ? 0.7 : 1

					Behavior on opacity {
						NumberAnimation {}
					}
				}

				Kirigami.Heading {
					text: root.chatController.rosterItem.displayName
					elide: Text.ElideMiddle
					Layout.fillWidth: true
					opacity: avatarActionButton.hovered ? 0.7 : 1

					Behavior on opacity {
						NumberAnimation {}
					}
				}
			}
			Layout.topMargin: - Kirigami.Units.smallSpacing
			Layout.fillWidth: true
			onClicked: {
				if (root.chatController.rosterItem.isProviderChat) {
					openOverlay(providerChatDetailsDialog)
				} else if (root.chatController.account.settings.jid === root.chatController.jid) {
					openOverlay(notesChatDetailsDialog)
				} else if (root.chatController.rosterItem.isGroupChat) {
					openOverlay(groupChatDetailsDialog)
				} else {
					openOverlay(contactDetailsDialog)
				}
			}
		},

		ToolbarCallButton {
			Controls.ToolTip.text: MainController.activeCall ? qsTr("Open audio call") : qsTr("Start audio call")
			source: "call-start-symbolic"
			visible: {
				if (!root.searchField.visible && root.chatController.account.connection.state === Enums.StateConnected && mediaDevices.audioInputs.length) {
					if (MainController.activeCall) {
						return MainController.activeCall.audioOnly
					}

					return !root.chatController.rosterItem.isGroupChat && !root.chatController.rosterItem.isNotesChat
				}

				return false
			}
			onClicked: {
				if (MainController.activeCall) {
					openPage(callPage)
				} else {
					root.chatController.account.callController.startAudioCall(root.chatController.jid)
				}
			}
		},

		ToolbarCallButton {
			Controls.ToolTip.text: MainController.activeCall ? qsTr("Open video call") : qsTr("Start video call")
			source: "camera-video-symbolic"
			visible: {
				if (!root.searchField.visible && root.chatController.account.connection.state === Enums.StateConnected && mediaDevices.videoInputs.length) {
					if (MainController.activeCall) {
						return !MainController.activeCall.audioOnly
					}

					return !root.chatController.rosterItem.isGroupChat && !root.chatController.rosterItem.isNotesChat
				}

				return false
			}
			onClicked: {
				if (MainController.activeCall) {
					openPage(callPage)
				} else {
					root.chatController.account.callController.startVideoCall(root.chatController.jid)
				}
			}
		},

		ToolbarButton {
			Controls.ToolTip.text: qsTr("Search upwards")
			source: "go-up-symbolic"
			opacity: root.searchField.opacity
			visible: root.searchField.visible
			onClicked: {
				root.searchFromCurrentIndex(true)
				root.searchField.forceActiveFocus()
			}
		},

		ToolbarButton {
			Controls.ToolTip.text: qsTr("Search downwards")
			source: "go-down-symbolic"
			opacity: root.searchField.opacity
			visible: root.searchField.visible
			onClicked: {
				root.searchFromCurrentIndex(false)
				root.searchField.forceActiveFocus()
			}
		},

		ToolbarButton {
			id: searchButton
			Controls.ToolTip.text: root.searchField.visible ? qsTr("Quit search") : qsTr("Search")
			source: "system-search-symbolic"
			checkable: true
		}
	]

	Component {
		id: providerChatDetailsDialog

		ProviderChatDetailsDialog {
			chatController: root.chatController
		}
	}

	Component {
		id: providerChatDetailsPage

		ProviderChatDetailsPage {
			chatController: root.chatController
		}
	}

	Component {
		id: notesChatDetailsDialog

		NotesChatDetailsDialog {
			chatController: root.chatController
		}
	}

	Component {
		id: notesChatDetailsPage

		NotesChatDetailsPage {
			chatController: root.chatController
		}
	}

	Component {
		id: notesChatDetailsKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			account: root.chatController.account
			Component.onDestruction: openView(notesChatDetailsDialog, notesChatDetailsPage)
		}
	}

	Component {
		id: contactDetailsDialog

		ContactDetailsDialog {
			chatController: root.chatController
		}
	}

	Component {
		id: contactDetailsPage

		ContactDetailsPage {
			chatController: root.chatController
		}
	}

	Component {
		id: contactDetailsAccountKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			account: root.chatController.account
			Component.onDestruction: openView(contactDetailsDialog, contactDetailsPage)
		}
	}

	Component {
		id: contactDetailsKeyAuthenticationPage

		ContactKeyAuthenticationPage {
			account: root.chatController.account
			jid: root.chatController.jid
			Component.onDestruction: openView(contactDetailsDialog, contactDetailsPage)
		}
	}

	Component {
		id: groupChatDetailsDialog

		GroupChatDetailsDialog {
			chatController: root.chatController
		}
	}

	Component {
		id: groupChatDetailsPage

		GroupChatDetailsPage {
			chatController: root.chatController
		}
	}

	Component {
		id: groupChatDetailsAccountKeyAuthenticationPage

		AccountKeyAuthenticationPage {
			account: root.chatController.account
			Component.onDestruction: openView(groupChatDetailsDialog, groupChatDetailsPage)
		}
	}

	Component {
		id: groupChatDetailsKeyAuthenticationPage

		GroupChatUserKeyAuthenticationPage {
			account: root.chatController.account
			Component.onDestruction: openView(groupChatDetailsDialog, groupChatDetailsPage).openKeyAuthenticationUserListView()
		}
	}

	MessageReactionEmojiPicker {
		id: messageReactionEmojiPicker
		messageModel: root.chatController.messageModel
		onClosed: {
			root.messageListView.restorePreviousCurrentIndex()
			root.sendingPane.forceActiveFocus()
		}
	}

	// View containing the messages
	ListView {
		id: messageListView

		property int previousCurrentIndex: currentIndex

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
		highlightMoveDuration: Kirigami.Units.longDuration
		// Connect to the database,
		model: root.chatController.messageModel
		visibleArea.onYPositionChanged: handleMessageRead()
		delegate: ChatMessage {
			messageSearchButton: searchButton
			messageListView: root.messageListView
			sendingPane: root.sendingPane
			reactionEmojiPicker: root.messageReactionEmojiPicker
			chatController: root.chatController
			modelIndex: index
			msgId: model.id
			senderJid: model.senderJid
			groupChatSenderId: model.groupChatSenderId
			senderName: model.senderName
			chatJid: root.chatController.jid
			chatName: root.chatController.rosterItem.displayName
			encryption: model.encryption
			trustLevel: model.trustLevel
			isGroupChatMessage: root.chatController.rosterItem.isGroupChat
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
			isLatestOldMessage: model.isLatestOldMessage
			edited: model.isEdited
			isSpoiler: model.isSpoiler
			spoilerHint: model.spoilerHint
			errorText: model.errorText
			files: model.files
			displayedReactions: model.displayedReactions
			detailedReactions: model.detailedReactions
			ownReactionsFailed: model.ownReactionsFailed
			groupChatInvitationJid: model.groupChatInvitationJid
			geoCoordinate: model.geoCoordinate
			marked: model.marked
		}
		// Everything is upside down, looks like a footer
		header: ColumnLayout {
			visible: root.chatController.account.settings.jid !== root.chatController.jid
			anchors.left: parent.left
			anchors.right: parent.right
			height: Kirigami.Units.largeSpacing * 3

			// chat state
			Controls.Label {
				text: root.chatController.chatStateText
				elide: Qt.ElideMiddle
				Layout.alignment: Qt.AlignCenter
				Layout.maximumWidth: parent.width - Kirigami.Units.largeSpacing * 4
			}
		}
		footer: ColumnLayout {
			z: 2
			spacing: 0
			width: parent.width

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
				elide: Text.ElideRight
				Layout.alignment: Qt.AlignHCenter
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
				Layout.maximumWidth: parent.width - Kirigami.Units.largeSpacing
				Component.onCompleted: root.globalChatDate = this
			}

			Item {
				height: Kirigami.Units.smallSpacing
			}
		}
		onActiveFocusChanged: {
			// This makes it possible on desktop devices to directly enter a message after opening
			// ChatPage.
			// The workaround is needed because messageListView's focus is automatically forced
			// after creation even when forcing sendingPane's focus within its
			// Component.onCompleted.
			if (activeFocus) {
				root.sendingPane.forceActiveFocus()
			}
		}

		// button for jumping to the latest message
		Controls.ItemDelegate {
			icon.name: "go-down-symbolic"
			background: Kirigami.ShadowedRectangle {
				color: {
					if (parent.pressed) {
						return Qt.darker(secondaryBackgroundColor, 1.05)
					}

					return parent.hovered ? secondaryBackgroundColor : primaryBackgroundColor
				}
				shadow.color: Qt.darker(color, 1.2)
				shadow.size: 4
				corners {
					topLeftRadius: height / 2
					topRightRadius: height / 2
					bottomLeftRadius: 0
					bottomRightRadius: 0
				}

				Behavior on color {
					ColorAnimation {
						duration: Kirigami.Units.shortDuration
					}
				}
			}
			topPadding: Kirigami.Units.smallSpacing * 3
			bottomPadding: topPadding
			leftPadding: Kirigami.Units.largeSpacing * 3
			rightPadding: leftPadding
			anchors.horizontalCenter: parent.horizontalCenter
			anchors.bottom: parent.bottom
			anchors.bottomMargin: parent.atYEnd ? - height - background.shadow.size - unreadMessageCounter.height : - background.shadow.size
			onClicked: parent.positionViewAtLatestMessage()

			Behavior on anchors.bottomMargin {
				SmoothedAnimation {}
			}

			MessageCounter {
				id: unreadMessageCounter
				count: root.chatController.rosterItem.unreadMessageCount
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.top
				anchors.verticalCenterOffset: -2
			}
		}

		Rectangle {
			id: forwardingInfo
			color: secondaryBackgroundColor
			opacity: root.chatController.messageBodyToForward ? 0.9 : 0
			visible: opacity
			anchors.fill: parent

			Behavior on opacity {
				NumberAnimation {}
			}

			ColumnLayout {
				spacing: Kirigami.Units.largeSpacing
				anchors.fill: parent

				Item {
					Layout.fillHeight: true
				}

				ClickableIcon {
					Controls.ToolTip.text: qsTr("Cancel forwarding")
					source: "window-close-symbolic"
					height: Kirigami.Units.iconSizes.enormous
					Layout.alignment: Qt.AlignHCenter
					onClicked: root.chatController.messageBodyToForward = ""
				}

				RowLayout {
					spacing: 0

					Item {
						Layout.fillWidth: true
					}

					ChatInfo {
						text: qsTr("Select a chat to forward the message")
						level: 1
						font.weight: Font.Medium
						wrapMode: Text.Wrap
						horizontalAlignment: Text.AlignHCenter
						Layout.maximumWidth: dropAreaInfo.width - Kirigami.Units.largeSpacing
					}

					Item {
						Layout.fillWidth: true
					}
				}

				Item {
					Layout.fillHeight: true
				}
			}
		}

		Rectangle {
			id: dropAreaInfo
			color: secondaryBackgroundColor
			opacity: 0
			visible: opacity
			anchors.fill: parent

			Behavior on opacity {
				NumberAnimation {}
			}

			ColumnLayout {
				spacing: Kirigami.Units.largeSpacing
				anchors.fill: parent

				Item {
					Layout.fillHeight: true
				}

				Kirigami.Icon {
					source: "mail-attachment-symbolic"
					height: Kirigami.Units.iconSizes.enormous
					Layout.alignment: Qt.AlignHCenter
				}

				RowLayout {
					spacing: 0

					Item {
						Layout.fillWidth: true
					}

					ChatInfo {
						text: qsTr("Drop files to be sent")
						level: 1
						font.weight: Font.Medium
						wrapMode: Text.Wrap
						horizontalAlignment: Text.AlignHCenter
						Layout.maximumWidth: dropAreaInfo.width - Kirigami.Units.largeSpacing
					}

					Item {
						Layout.fillWidth: true
					}
				}

				RowLayout {
					spacing: 0

					Item {
						Layout.fillWidth: true
					}

					ChatInfo {
						text: filePastingShortcut.nativeText
						level: 5
						wrapMode: Text.Wrap
						horizontalAlignment: Text.AlignHCenter
						Layout.maximumWidth: dropAreaInfo.width - Kirigami.Units.largeSpacing
					}

					Item {
						Layout.fillWidth: true
					}
				}

				Item {
					Layout.fillHeight: true
				}
			}
		}

		DropArea {
			id: messageListViewDropArea
			anchors.fill: parent
			onDropped: drop => root.addDroppedFiles(drop)
			onContainsDragChanged: {
				if (!chatHintListViewDropArea.containsDrag) {
					dropAreaInfo.opacity = containsDrag ? 0.9 : 0
				}
			}
		}

		Timer {
			id: resetCurrentIndexTimer
			interval: Kirigami.Units.veryLongDuration * 4
			onTriggered: messageListView.resetCurrentIndex()
		}

		Connections {
			target: root.chatController.messageModel

			function onMessageFetchingFinished() {
				// Skip the case when messages are fetched after the initial fetching because this
				// function positioned the view at firstUnreadContactMessageIndex and that is close
				// to the end of the loaded messages.
				if (!root.viewPositioned) {
					if (root.chatController.rosterItem.unreadMessageCount) {
						const firstUnreadContactMessageIndex = root.chatController.messageModel.firstUnreadContactMessageIndex()

						if (firstUnreadContactMessageIndex > 0) {
							root.messageListView.positionViewAtIndex(firstUnreadContactMessageIndex, ListView.End)
						}

						// Trigger sending read markers manually as the view is ready.
						root.messageListView.handleMessageReadOnPositionedView()
					}

					root.viewPositioned = true

					root.sendingPane.setCurrentItemToMessageBeingCorrected()
				}
			}
		}

		Connections {
			target: Qt.application

			function onStateChanged(state) {
				// Send a read marker once the application becomes active if a message has been received while the application was not active.
				if (state === Qt.ApplicationActive) {
					root.messageListView.handleMessageRead()
				}
			}
		}

		function positionViewAtLatestMessage() {
			positionViewAtIndex(0, ListView.Center)
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
			const distanceToLatestMessageBottom = height + contentY
			const messageBottomOffset = 10
			const readMessageIndex = indexAt(0, (distanceToLatestMessageBottom + messageBottomOffset)) + 1

			root.chatController.messageModel.handleMessageRead(readMessageIndex)
		}

		function highlightShortly(index) {
			currentIndex = index
			resetCurrentIndexTimer.restart()
		}

		function setCurrentIndex(index) {
			previousCurrentIndex = currentIndex
			currentIndex = index
		}

		function restorePreviousCurrentIndex() {
			currentIndex = previousCurrentIndex
		}

		function resetCurrentIndex() {
			currentIndex = -1
		}
	}

	footer:	ListView {
		id: chatHintListView
		height: contentHeight
		verticalLayoutDirection: ListView.BottomToTop
		interactive: false
		headerPositioning: ListView.OverlayHeader
		model: root.chatController.chatHintModel
		delegate: ChatHintArea {
			id: chatHintArea
			chatHintModel: ListView.view.model
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
			height: root.chatController.rosterItem.isDeletedGroupChat ? 0 : undefined
			Component.onCompleted: root.sendingPane = this
		}

		DropArea {
			id: chatHintListViewDropArea
			anchors.fill: parent
			onDropped: drop => root.addDroppedFiles(drop)
			onContainsDragChanged: {
				if (!messageListViewDropArea.containsDrag) {
					dropAreaInfo.opacity = containsDrag ? 0.9 : 0
				}
			}
		}
	}

	// Shortcut to show search field.
	Shortcut {
		sequence: root.searchField.focusSequence
		enabled: !searchButton.checked
		onActivated: searchButton.clicked()
	}

	Shortcut {
		id: filePastingShortcut
		sequence: "Ctrl+Shift+V"
		onActivated: {
			const url = MediaUtils.urlFromClipboard()

			// Check if there is a file to be pasted from the clipboard.
			if (url.toString()) {
				addFile(url)
			}
		}
	}

	MediaDevices {
		id: mediaDevices
	}

	Connections {
		target: root.messageListView.model

		function onMessageSearchFinished(queryStringMessageIndex) {
			if (queryStringMessageIndex !== -1) {
				root.messageListView.currentIndex = queryStringMessageIndex
			}

			root.searchField.busy = false
		}
	}

	Connections {
		target: root.chatController.account ? root.chatController.account.groupChatController : null

		function onGroupChatInviteeSelectionNeeded() {
			openView(groupChatDetailsDialog, groupChatDetailsPage).openInviteeListView()
		}
	}

	// Needs to be outside of the DetailsDialog to not be destroyed with it.
	// Otherwise, the undo action of "showPassiveNotification()" would point to a destroyed object.
	Connections {
		target: root.chatController.account ? root.chatController.account.blockingController : null

		function onUnblocked(jid) {
			// Show a passive notification when a JID that is not in the roster is unblocked and
			// provide an option to undo that.
			// JIDs in the roster can be blocked again via their details.
			if (!RosterModel.hasItem(root.chatController.account.settings.jid, jid)) {
				showPassiveNotification(qsTr("Unblocked %1").arg(jid), "long", qsTr("Undo"), () => {
					root.chatController.account.blockingController.block(jid)
				})
			}
		}

		function onBlockingFailed(jid, errorText) {
			showPassiveNotification(qsTr("Could not block %1: %2").arg(jid).arg(errorText))
		}

		function onUnblockingFailed(jid, errorText) {
			showPassiveNotification(qsTr("Could not unblock %1: %2").arg(jid).arg(errorText))
		}
	}

	function initialize(accountJid, chatJid) {
		viewPositioned = false
		chatController.initialize(AccountController.account(accountJid), chatJid)
	}

	/**
	 * Searches for a message containing the entered text in the search field starting from the current index of the message list view.
	 *
	 * The searchField is automatically focused again on desktop devices if it lost focus (e.g., after clicking a button).
	 *
	 * @param searchUpwards true for searching upwards or false for searching downwards
	 */
	function searchFromCurrentIndex(searchUpwards) {
		if (!Kirigami.Settings.isMobile && !searchField.activeFocus) {
			searchField.forceActiveFocus()
		}

		search(searchUpwards, messageListView.currentIndex + (searchUpwards ? 1 : -1))
	}

	/**
	 * Searches for a message containing the entered text in the search field.
	 *
	 * If a message is found for the entered text, that message is highlighted.
	 *
	 * @param searchUpwards true for searching upwards or false for searching downwards
	 * @param startIndex index of the first message to search for the entered text
	 */
	function search(searchUpwards, startIndex) {
		let newIndex = -1
		const searchedString = searchField.text

		if (searchedString) {
			root.searchField.busy = true
			let messageModel = messageListView.model

			if (searchUpwards) {
				if (startIndex === 0) {
					messageListView.currentIndex = messageModel.searchForMessageFromNewToOld(searchedString)
				} else {
					newIndex = messageModel.searchForMessageFromNewToOld(searchedString, startIndex)

					if (newIndex !== -1) {
						messageListView.currentIndex = newIndex
					}
				}

				if (messageListView.currentIndex !== -1) {
					root.searchField.busy = false
				}
			} else {
				newIndex = messageModel.searchForMessageFromOldToNew(searchedString, startIndex)

				if (newIndex !== -1) {
					messageListView.currentIndex = newIndex
				}

				root.searchField.busy = false
			}
		} else {
			messageListView.currentIndex = newIndex
		}
	}

	function addDroppedFiles(drop) {
		for (const url of drop.urls) {
			addFile(url)
		}
	}

	function addFile(url) {
		sendingPane.composition.fileSelectionModel.addFile(url)
	}

	function forceActiveFocus() {
		sendingPane.forceActiveFocus()
	}
}
