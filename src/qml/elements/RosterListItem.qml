// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

UserListItem {
	id: root

	property ListView listView
	property Controls.Menu contextMenu
	property bool lastMessageIsDraft
	property alias lastMessageDateTime: lastMessageDateTimeText.text
	property string lastMessage
	property string lastMessageSenderId
	property int unreadMessages
	property bool pinned
	property bool notificationsMuted

	selected: {
		return !Kirigami.Settings.isMobile &&
			MessageModel.currentAccountJid === accountJid &&
			MessageModel.currentChatJid === jid
	}

	// middle
	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing

		RowLayout {
			// name
			Kirigami.Heading {
				id: nameText
				text: root.name
				textFormat: Text.PlainText
				elide: Text.ElideRight
				maximumLineCount: 1
				level: 4
				Layout.maximumHeight: Kirigami.Units.gridUnit * 1.5
				Layout.fillWidth: true
			}

			// last (exchanged/draft) message date/time
			Text {
				id: lastMessageDateTimeText
				text: root.lastMessageDateTime
				visible: text && root.lastMessage
				color: Kirigami.Theme.disabledTextColor
				font.weight: Font.Light
			}
		}

		RowLayout {
			// prefix of the last message
			Controls.Label {
				id: lastMessagePrefix
				visible: text && (lastMessageIsDraft || lastMessageSenderId)
				textFormat: Text.PlainText
				text: {
					if (lastMessageIsDraft) {
						return qsTr("Draft:")
					} else {
						// Omit the sender in case of the chat with oneself.
						if (root.jid == root.accountJid) {
							return ""
						}

						if (lastMessageSenderId === root.accountJid) {
							return qsTr("Me:")
						}

						return qsTr("%1:").arg(root.name)
					}
				}
				color: Kirigami.Theme.disabledTextColor
				font {
					weight: Font.Light
					italic: lastMessageIsDraft
				}
			}

			// last message or error status message
			Controls.Label {
				id: lastMessageText
				Layout.fillWidth: true
				elide: Text.ElideRight
				maximumLineCount: 1
				text: Utils.removeNewLinesFromString(lastMessage)
				textFormat: Text.PlainText
				font.weight: Font.Light
			}

			// icon for muted contact
			Kirigami.Icon {
				source: "notifications-disabled-symbolic"
				visible: root.notificationsMuted
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: counter.height
			}

			// icon for pinned chat
			Kirigami.Icon {
				source: "window-pin-symbolic"
				visible: root.pinned
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: counter.height
			}

			// unread message counter
			MessageCounter {
				id: counter
				count: root.unreadMessages
				muted: root.notificationsMuted
			}

			// icon for reordering
			Kirigami.ListItemDragHandle {
				visible: root.pinned
				listItem: root
				listView: root.listView
				onMoveRequested: RosterModel.reorderPinnedItem(root.accountJid, root.jid, oldIndex, newIndex)
				Layout.preferredWidth: Layout.preferredHeight
				Layout.preferredHeight: counter.height
			}
		}
	}

	MouseArea {
		parent: root
		anchors.fill: parent
		acceptedButtons: Qt.RightButton

		onClicked: {
			if (mouse.button === Qt.RightButton) {
				showContextMenu()
			}
		}

		onPressAndHold: showContextMenu()
	}

	function showContextMenu() {
		if (contextMenu) {
			contextMenu.item = this
			contextMenu.popup()
		}
	}
}
