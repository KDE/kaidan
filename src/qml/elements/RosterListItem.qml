// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

UserListItem {
	id: root

	property ListView listView
	property bool isPublicGroupChat
	property bool isDeletedGroupChat
	property alias lastMessageDateTime: lastMessageDateTimeText.text
	property string lastMessage
	property bool lastMessageIsDraft
	property bool lastMessageIsOwn
	property string lastMessageGroupChatSenderName
	property int unreadMessages
	property bool pinModeActive
	property bool pinned
	property int notificationRule

	signal moveRequested(int oldIndex, int newIndex)
	signal dropRequested(int oldIndex, int newIndex)

	selected: {
		return !Kirigami.Settings.isMobile &&
			ChatController.accountJid === accountJid &&
			ChatController.chatJid === jid
	}
	dragged: dragHandle.dragActive

	ColumnLayout {
		id: middleArea
		spacing: 0
		Layout.topMargin: parent.spacing
		Layout.bottomMargin: Layout.topMargin

		RowLayout {
			Layout.preferredHeight: parent.height / 2

			// group chat icon
			Kirigami.Icon {
				source: "system-users-symbolic"
				visible: root.isGroupChat
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: nameText.height
			}

			// name
			Kirigami.Heading {
				id: nameText
				text: root.name
				textFormat: Text.PlainText
				elide: Text.ElideRight
				maximumLineCount: 1
				type: root.unreadMessages ? Kirigami.Heading.Type.Primary : Kirigami.Heading.Type.Normal
				level: 4
				Layout.fillWidth: true
			}

			// last (exchanged/draft) message date/time
			Text {
				id: lastMessageDateTimeText
				text: root.lastMessageDateTime
				visible: text && root.lastMessage && !root.isDeletedGroupChat
				color: Kirigami.Theme.disabledTextColor
				font.weight: Font.Light
				font.pixelSize: Kirigami.Theme.defaultFont.pixelSize * 0.95
			}

			Text {
				text: isDeletedGroupChat ? qsTr("Deleted") : ""
				visible: root.isDeletedGroupChat
				color: Kirigami.Theme.negativeTextColor
				font: lastMessageDateTimeText.font
			}
		}

		Item {
			Layout.fillHeight: true
		}

		RowLayout {
			id: secondRow
			width: parent.width
			Layout.preferredHeight: parent.height / 2

			// placeholder if no messages have been exchanged yet
			Controls.Label {
				text: qsTr("No messages yet")
				visible: !root.lastMessage
				color: Kirigami.Theme.disabledTextColor
				elide: Text.ElideRight
				font {
					weight: Font.Light
					italic: true
				}
				Layout.fillWidth: true
			}

			// prefix of the last message
			Controls.Label {
				id: lastMessagePrefix
				text: {
					if (root.lastMessageIsDraft) {
						return qsTr("Draft:")
					} else {
						if (root.lastMessageIsOwn) {
							return qsTr("Me:")
						}

						if (root.lastMessageGroupChatSenderName) {
							return qsTr("%1:").arg(root.lastMessageGroupChatSenderName)
						}

						return ""
					}
				}
				visible: text
				textFormat: Text.PlainText
				color: root.lastMessageIsDraft ? Kirigami.Theme.activeTextColor : Kirigami.Theme.disabledTextColor
				font {
					weight: Font.Light
					italic: root.lastMessageIsDraft
				}
				elide: Text.ElideMiddle
				Layout.preferredWidth: implicitWidth
				Layout.maximumWidth: Kirigami.Units.gridUnit * 6
				// Ensure the same spacing between lastMessagePrefix and lastMessageText regardless
				// of whether lastMessagePrefix is elided.
				Layout.rightMargin: implicitWidth > width ? - Kirigami.Units.mediumSpacing : 0
			}

			// last message or error status message
			FormattedTextEdit {
				id: lastMessageText
				text: lastMessageTextMetrics.elidedText
				visible: root.lastMessage
				color: root.lastMessageIsDraft ? Kirigami.Theme.activeTextColor : Kirigami.Theme.textColor
				wrapMode: Text.NoWrap
				font.weight: root.unreadMessages ? Font.Normal : Font.Light
				Layout.fillWidth: true

				TextMetrics {
					id: lastMessageTextMetrics
					text: Utils.removeNewLinesFromString(root.lastMessage)
					elide: Text.ElideRight
					elideWidth: {
						let width = middleArea.width

						for (let i in secondRow.visibleChildren) {
							let visibleChild = secondRow.visibleChildren[i]
							if (visibleChild !== lastMessageText) {
								width -= secondRow.spacing + visibleChild.width
							}
						}

						return width
					}
				}
			}

			// icon for muted roster item
			Kirigami.Icon {
				id: mutedIcon
				source: "notifications-disabled-symbolic"
				visible: root.notificationRule === RosterItem.NotificationRule.Never
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: counter.height
			}

			// icon-like text for roster item that notifies only when user is mentioned in group
			// chat
			ScalableText {
				id: mentionIcon
				text: "@"
				visible: root.notificationRule === RosterItem.NotificationRule.Mentioned
				scaleFactor: counter.height * 0.065
				Layout.topMargin: - 2
			}

			// unread message counter
			MessageCounter {
				id: counter
				count: root.unreadMessages
				muted: root.notificationRule === RosterItem.NotificationRule.Never ||
					   root.notificationRule === RosterItem.NotificationRule.Mentioned
			}
		}
	}

	// handle for reordering a pinned roster item
	Kirigami.ListItemDragHandle {
		id: dragHandle
		visible: root.pinModeActive && root.pinned
		listItem: root.contentItem
		listView: root.listView
		incrementalMoves: false

		onMoveRequested: (oldIndex, newIndex) => root.moveRequested(oldIndex, newIndex)
		onDropped: (oldIndex, newIndex) => root.dropRequested(oldIndex, newIndex)

		Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
		Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
	}

	ClickableIcon {
		Controls.ToolTip.text: root.pinned ? qsTr("Unpin") : qsTr("Pin")
		source: root.pinned ? "starred-symbolic" : "non-starred-symbolic"
		visible: root.pinModeActive
		Layout.preferredWidth: Kirigami.Units.iconSizes.smallMedium
		Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
		onClicked: {
			if (root.pinned) {
				RosterModel.unpinItem(root.accountJid, root.jid)
			} else {
				RosterModel.pinItem(root.accountJid, root.jid)
			}
		}
	}
}
