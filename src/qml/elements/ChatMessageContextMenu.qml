// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2020 Yury Gubich <blue@macaw.me>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a context menu with entries used for chat messages.
 */
Kirigami.Dialog {
	id: root

	required property Item message
	property var file: null

	padding: Kirigami.Units.smallSpacing * 3
	background: Kirigami.ShadowedRectangle {
		color: primaryBackgroundColor
		radius: height
		shadow.color: Qt.darker(color, 2)
		shadow.size: 6
	}
	header: Item {}
	footer: Item {}
	// Ensure that the whole content is always inside of the visible area.
	onImplicitWidthChanged: position(x, y)
	onClosed: destroy()

	Behavior on implicitWidth {
		SmoothedAnimation {
			duration: Kirigami.Units.longDuration
		}
	}

	RowLayout {
		id: contentArea
		spacing: Kirigami.Units.largeSpacing * 2

		ChatMessageContextMenuButton {
			id: fileRemovalButton
			Controls.ToolTip.text: qsTr("Remove selected file")
			source: "list-remove-symbolic"
			contextMenu: root
			shown: root.file && root.file.localFilePath
			onClicked: Kaidan.fileSharingController.deleteFile(root.message.msgId, root.file)
		}

		Kirigami.Separator {
			visible: fileRemovalButton.visible
			Layout.leftMargin: - Kirigami.Units.smallSpacing
			Layout.fillHeight: true
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("React")
			source:  "smiley-add"
			contextMenu: root
			shown: {
				// Do not allow to send reactions if the message has not yet got a stanza ID from
				// the server to be referenced by.
				if (root.message.isGroupChatMessage &&
					root.message.deliveryState !== Enums.Sent &&
					root.message.deliveryState !== Enums.Delivered) {
					return false
				}

				return !root.message.displayedReactions.length && !root.message.groupChatInvitationJid
			}
			Layout.preferredWidth: Kirigami.Units.iconSizes.medium
			Layout.preferredHeight: Kirigami.Units.iconSizes.medium
			Layout.topMargin: - Kirigami.Units.smallSpacing
			Layout.bottomMargin: Layout.topMargin
			Layout.leftMargin: fileRemovalButton.visible ? - Kirigami.Units.largeSpacing : - Kirigami.Units.smallSpacing
			Layout.rightMargin: - Kirigami.Units.smallSpacing
			onClicked: {
				root.message.reactionEmojiPicker.messageId = root.message.msgId
				root.message.reactionEmojiPicker.open()
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

				return !root.message.groupChatInvitationJid
			}
			onClicked: root.message.sendingPane.prepareReply(root.message.senderJid, root.message.groupChatSenderId, root.message.senderName, root.message.msgId, root.message.messageBody)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Quote")
			source: "mail-reply-all-symbolic"
			contextMenu: root
			shown: root.message.messageBody && !root.message.groupChatInvitationJid
			onClicked: root.message.sendingPane.prepareQuote(root.message.messageBody)
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
			shown: !root.message.groupChatInvitationJid && MessageModel.canCorrectMessage(root.message.modelIndex)
			onClicked: root.message.sendingPane.prepareCorrection(root.message.msgId, root.message.replyToJid, root.message.replyToGroupChatParticipantId, root.message.replyToName, root.message.replyId, root.message.replyQuote, root.message.messageBody, root.message.spoilerHint)
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Mark as first unread")
			source: "mail-mark-important-symbolic"
			contextMenu: root
			shown: !root.message.isOwn
			onClicked: {
				MessageModel.markMessageAsFirstUnread(message.modelIndex);
				ChatController.resetChat()
				Kaidan.closeChatPageRequested()
			}
		}

		ChatMessageContextMenuButton {
			Controls.ToolTip.text: qsTr("Remove from this device")
			source: "edit-delete-symbolic"
			contextMenu: root
			onClicked: {
				MessageModel.removeMessage(root.message.msgId)

				if (root.file && root.file.localFilePath) {
					Kaidan.fileSharingController.deleteFile(root.message.msgId, root.file)
				}
			}
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

	function show(mouseArea, file) {
		if (file === undefined) {
			root.file = null
		} else {
			root.file = file
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
