// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a context menu with entries used for chat messages.
 */
Kirigami.Dialog {
	id: root

	required property Item message
	property var file: null
	readonly property bool localFileAvailable: file && file.locallyAvailable
	property bool currentIndexResetOnClosing: true

	padding: Kirigami.Units.smallSpacing * 3
	background: Kirigami.ShadowedRectangle {
		color: primaryBackgroundColor
		radius: height / 2
		shadow.color: Qt.darker(color, 2)
		shadow.size: 6
	}
	header: null
	footer: null
	// Ensure that the whole content is always inside of the visible area.
	onImplicitWidthChanged: position(x, y)
	onClosed: {
		if (currentIndexResetOnClosing) {
			message.messageListView.restorePreviousCurrentIndex()
		}

		destroy()
	}

	Behavior on implicitWidth {
		SmoothedAnimation {
			duration: Kirigami.Units.longDuration
		}
	}

	RowLayout {
		id: contentArea
		spacing: Kirigami.Units.largeSpacing * 2
		// Fix no buttons being shown.
		implicitHeight: 1

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Remove file")
			source: "list-remove-symbolic"
			contextMenu: root
			shown: root.localFileAvailable
			onClicked: message.chatController.messageModel.deleteFile(root.message.modelIndex, file)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Open file in folder")
			source: "folder-symbolic"
			contextMenu: root
			shown: root.localFileAvailable
			onClicked: MediaUtils.openFileInFolder(root.file.localFileUrl)
		}

		Kirigami.Separator {
			visible: root.localFileAvailable
			Layout.fillHeight: true
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Resend")
			source: "view-refresh-symbolic"
			contextMenu: root
			shown: root.message.deliveryState === Enums.Error
			onClicked: root.message.chatController.messageModel.resendMessage(root.message.modelIndex)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("React")
			source:  "emoji-people-symbolic"
			fallback: "smiley-add"
			contextMenu: root
			shown: {
				// Do not allow to send reactions if the message has not yet got a stanza ID from
				// the server to be referenced by.
				if (root.message.isGroupChatMessage &&
					root.message.deliveryState !== Enums.Sent &&
					root.message.deliveryState !== Enums.Delivered) {
					return false
				}

				return !root.message.displayedReactions.length && !root.message.groupChatInvitationJid && !root.message.chatController.rosterItem.isDeletedGroupChat
			}
			onClicked: {
				let reactionEmojiPicker = root.message.reactionEmojiPicker
				reactionEmojiPicker.messageId = root.message.msgId
				reactionEmojiPicker.open()

				root.currentIndexResetOnClosing = false
			}
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Reply")
			source: "mail-reply-sender-symbolic"
			contextMenu: root
			shown: {
				// Do not allow to reply if the message has not yet got a stanza ID from the
				// server to be referenced by.
				if (root.message.isGroupChatMessage &&
					root.message.deliveryState !== Enums.Sent &&
					root.message.deliveryState !== Enums.Delivered) {
					return false
				}

				return !root.message.groupChatInvitationJid && !root.message.chatController.rosterItem.isDeletedGroupChat
			}
			onClicked: root.message.sendingPane.prepareReply(root.message.senderJid, root.message.groupChatSenderId, root.message.senderName, root.message.msgId, root.message.messageBody)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Quote")
			source: "mail-reply-all-symbolic"
			contextMenu: root
			shown: root.message.messageBody && !root.message.groupChatInvitationJid && !root.message.chatController.rosterItem.isDeletedGroupChat
			onClicked: root.message.sendingPane.prepareQuote(root.message.messageBody)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Forward")
			source: "mail-forward-symbolic"
			contextMenu: root
			shown: root.message.messageBody
			onClicked: root.message.chatController.messageBodyToForward = root.message.messageBody
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Copy")
			source: "edit-copy-symbolic"
			contextMenu: root
			shown: root.message.messageBody || root.message.spoilerHint
			onClicked: {
				if (!root.message.isSpoiler || root.message && root.message.isShowingSpoiler) {
					Utils.copyToClipboard(root.message.messageBody)
				} else {
					Utils.copyToClipboard(root.message.spoilerHint)
				}
			}
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Edit")
			source: "document-edit-symbolic"
			contextMenu: root
			shown: root.message.chatController.messageModel.canCorrectMessage(root.message.modelIndex)
			onClicked: {
				root.message.sendingPane.prepareCorrection(root.message.msgId, root.message.replyToJid, root.message.replyToGroupChatParticipantId, root.message.replyToName, root.message.replyId, root.message.replyQuote, root.message.messageBody, root.message.spoilerHint)
				root.currentIndexResetOnClosing = false
			}
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Mark")
			source: "mail-mark-important-symbolic"
			contextMenu: root
			shown: !root.message.marked
			onClicked: root.message.chatController.messageModel.setMessageMarked(root.message.modelIndex, true)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Remove from this device")
			source: "edit-delete-symbolic"
			contextMenu: root
			onClicked: root.message.chatController.messageModel.removeMessage(root.message.msgId)
		}
	}

	function expandButtons() {
		const contentAreaChildren = contentArea.children

		for (let i in contentAreaChildren) {
			let child = contentAreaChildren[i]

			if (child instanceof ChatMessageContextMenuButton) {
				let expansionTimer = child.expansionTimer
				expansionTimer.interval = Kirigami.Units.veryShortDuration * 0.1
				expansionTimer.start()
			}
		}
	}

	function show(mouseArea, selectedFile) {
		message.messageListView.setCurrentIndex(root.message.modelIndex)

		if (selectedFile === undefined) {
			file = null
		} else {
			file = selectedFile
		}

		const absolutePosition = mouseArea.mapToGlobal(mouseArea.mouseX, mouseArea.mouseY)
		position(absolutePosition.x, absolutePosition.y)

		expandButtons()
		open()
	}

	function position(xPosition, yPosition) {
		const messageListView = message.messageListView
		const messageListViewRightEdgeX = messageListView.mapToGlobal(messageListView.x, messageListView.y).x + messageListView.width
		const rightEdgeX = xPosition + implicitWidth

		if (rightEdgeX >= messageListViewRightEdgeX) {
			const hiddenWidth = rightEdgeX - messageListViewRightEdgeX
			x = xPosition - hiddenWidth
		} else {
			x = xPosition
		}

		y = yPosition
	}
}
