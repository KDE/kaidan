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

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtMultimedia 5.14 as Multimedia
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
	property alias messageReactionDetailsSheet: messageReactionDetailsSheet

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
				jid: MessageModel.currentChatJid
				name: chatItemWatcher.item.displayName
			}
			Kirigami.Heading {
				Layout.fillWidth: true
				Layout.leftMargin: Kirigami.Units.largeSpacing
				Layout.rightMargin: Kirigami.Units.largeSpacing
				text: chatItemWatcher.item.displayName
			}
		}

		onClicked: openOverlay(contactDetailsSheet)
	}
	keyboardNavigationEnabled: true
	contextualActions: [
		Kirigami.Action {
			visible: Kirigami.Settings.isMobile
			icon.name: "avatar-default-symbolic"
			text: qsTr("Details…")
			onTriggered: openPage(contactDetailsPage)
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
			onTriggered: sendingPane.composition.isSpoiler = true
		}
	]

	// Message search bar
	header: ChatPageSearchView {
		id: searchBar
	}

	RosterItemWatcher {
		id: chatItemWatcher
		jid: MessageModel.currentChatJid
	}

	Component {
		id: contactDetailsSheet

		ContactDetailsSheet {
			accountJid: MessageModel.currentAccountJid
			jid: MessageModel.currentChatJid
		}
	}

	Component {
		id: contactDetailsPage

		ContactDetailsPage {
			accountJid: MessageModel.currentAccountJid
			jid: MessageModel.currentChatJid
		}
	}

	Component {
		id: contactDetailsKeyAuthenticationPage

		KeyAuthenticationPage {
			Component.onDestruction: openView(contactDetailsSheet, contactDetailsPage)
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

	MessageReactionDetailsSheet {
		id: messageReactionDetailsSheet
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
			id: highlightBar
			Rectangle {
				height: messageListView.currentIndex === -1 ? 0 : messageListView.currentItem.implicitHeight
				width: messageListView.currentIndex === -1 ? 0 : messageListView.currentItem.width + Kirigami.Units.smallSpacing * 2
				color: Kirigami.Theme.hoverColor

				// This is used to make the highlight bar a little bit bigger than the highlighted message.
				// It works only together with "messageListView.highlightFollowsCurrentItem: false".
				y: messageListView.currentIndex === -1 ? 0 : messageListView.currentItem.y
				x: messageListView.currentIndex === -1 ? 0 : messageListView.currentItem.x
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
					let unreadMessageCount = chatItemWatcher.item.unreadMessageCount

					if (unreadMessageCount) {
						let firstUnreadContactMessageIndex = MessageModel.firstUnreadContactMessageIndex()

						if (firstUnreadContactMessageIndex > 0) {
							messageListView.positionViewAtIndex(firstUnreadContactMessageIndex, ListView.End)
						}

						root.viewPositioned = true

						// Trigger sending read markers manually as the view is ready.
						messageListView.handleMessageRead()
					} else {
						root.viewPositioned = true
					}
				}
			}

			function onMessageSearchFinished(queryStringMessageIndex) {
				if (queryStringMessageIndex !== -1) {
					messageListView.currentIndex = queryStringMessageIndex
				}

				searchBar.searchFieldBusyIndicator.running = false
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
		 * Sends a read marker for the latest visible / read message.
		 */
		function handleMessageRead() {
			if (root.viewPositioned) {
				MessageModel.handleMessageRead(indexAt(0, (contentY + height + 15)) + 1)
			}
		}

		ChatMessageContextMenu {
			id: messageContextMenu
		}

		delegate: ChatMessage {
			contextMenu: messageContextMenu
			reactionEmojiPicker: root.messageReactionEmojiPicker
			reactionDetailsSheet: root.messageReactionDetailsSheet
			modelIndex: index
			msgId: model.id
			senderId: model.senderId
			senderName: model.isOwn ? "" : chatItemWatcher.item.displayName
			chatName: chatItemWatcher.item.displayName
			encryption: model.encryption
			isTrusted: model.isTrusted
			isOwn: model.isOwn
			messageBody: model.body
			date: model.date
			time: model.time
			deliveryState: model.deliveryState
			deliveryStateName: model.deliveryStateName
			deliveryStateIcon: model.deliveryStateIcon
			isLastRead: model.isLastRead
			edited: model.isEdited
			isSpoiler: model.isSpoiler
			spoilerHint: model.spoilerHint
			errorText: model.errorText
			files: model.files
			displayedReactions: model.displayedReactions
			detailedReactions: model.detailedReactions
			ownDetailedReactions: model.ownDetailedReactions

			onMessageEditRequested: (replaceId, body, spoilerHint) => sendingPane.prepareMessageCorrection(replaceId, body, spoilerHint)

			onQuoteRequested: {
				let quotedText = ""
				const lines = body.split("\n")

				for (let line in lines) {
					quotedText += "> " + lines[line] + "\n"
				}

				sendingPane.messageArea.insert(0, quotedText)
			}
		}

		// Everything is upside down, looks like a footer
		header: ColumnLayout {
			visible: MessageModel.currentAccountJid !== MessageModel.currentChatJid
			anchors.left: parent.left
			anchors.right: parent.right
			height: stateLabel.text ? 20 : 0

			Controls.Label {
				id: stateLabel
				Layout.alignment: Qt.AlignCenter
				Layout.maximumWidth: parent.width
				height: !text ? 20 : 0
				topPadding: text ? 10 : 0

				text: Utils.chatStateDescription(chatItemWatcher.item.displayName, MessageModel.chatState)
				elide: Qt.ElideMiddle
			}
		}

		footer: ColumnLayout {
			z: 2
			anchors.horizontalCenter: parent.horizontalCenter
			spacing: 0

			Item {
				height: Kirigami.Units.smallSpacing * 3
			}

			// date of the top-most (first) visible message shown at the top of the ChatPage
			ChatInfo {
				text: {
					// The "yPosition" is checked in order to update/display the text once the
					// ChatPage is opened and its messages loaded.
					if (messageListView.visibleArea.yPosition >= 0) {
						const contentY = messageListView.contentY

						// Search until a message is found if there is a section label instead of a
						// message at the top of the visible/content area.
						// "i" is used as an offset.
						for (let i = 0; i < 100; i++) {
							const firstVisibleMessage = messageListView.itemAt(0, contentY + i)

							if (firstVisibleMessage) {
								return firstVisibleMessage.date
							}
						}

						return ""
					}

					return ""
				}
				visible: text.length
				Component.onCompleted: root.globalChatDate = this
			}

			Controls.BusyIndicator {
				visible: opacity !== 0.0
				height: visible ? undefined : Kirigami.Units.smallSpacing * 4
				Layout.alignment: Qt.AlignHCenter
				Layout.topMargin: Kirigami.Units.smallSpacing
				padding: 0
				opacity: MessageModel.mamLoading ? 1.0 : 0.0

				Behavior on opacity {
					NumberAnimation {
						duration: Kirigami.Units.shortDuration
					}
				}
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
				count: chatItemWatcher.item.unreadMessageCount
				anchors.horizontalCenter: parent.horizontalCenter
				anchors.verticalCenter: parent.top
				anchors.verticalCenterOffset: -2
			}
		}
	}

	footer:	ListView {
		id: chatHintListView
		height: contentHeight
		verticalLayoutDirection: ListView.BottomToTop
		interactive: false
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

			Component.onCompleted: {
				root.sendingPane = this
			}
		}
		onCountChanged: {
			// Make it possible to directly enter a message after chatHintListView is focused because of changes in its model.
			sendingPane.forceActiveFocus()
		}
	}
}
