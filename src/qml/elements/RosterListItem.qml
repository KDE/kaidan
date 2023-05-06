// SPDX-FileCopyrightText: 2017 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiragahv@gmail.com>
// SPDX-FileCopyrightText: 2022 Bhavy Airi <airiraghav@gmail.com>
// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
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
	property string lastMessage
	property int unreadMessages
	property bool pinned
	property bool notificationsMuted

	isSelected: {
		return !Kirigami.Settings.isMobile &&
			MessageModel.currentAccountJid === accountJid &&
			MessageModel.currentChatJid === jid
	}

	// middle
	ColumnLayout {
		spacing: Kirigami.Units.largeSpacing
		Layout.fillWidth: true

		// name
		Kirigami.Heading {
			id: nameText
			text: name
			textFormat: Text.PlainText
			elide: Text.ElideRight
			maximumLineCount: 1
			level: 4
			Layout.fillWidth: true
			Layout.maximumHeight: Kirigami.Units.gridUnit * 1.5
		}

		// last message or error status message if available, otherwise not visible
		RowLayout {
			visible: lastMessageText.text

			Layout.fillWidth: true

			Controls.Label {
				id: draft
				visible: lastMessageIsDraft
				textFormat: Text.PlainText
				text: qsTr("Draft:")
				font {
					weight: Font.Light
					italic: true
				}
			}

			Controls.Label {
				id: lastMessageText
				Layout.fillWidth: true
				elide: Text.ElideRight
				maximumLineCount: 1
				text: Utils.removeNewLinesFromString(lastMessage)
				textFormat: Text.PlainText
				font.weight: Font.Light
			}
		}
	}

	onIsSelectedChanged: textColorAnimation.restart()

	// fading text colors
	ColorAnimation {
		id: textColorAnimation
		targets: [nameText, lastMessageText]
		property: "color"
		to: root.isSelected ? Kirigami.Theme.textColor : Kirigami.Theme.highlightedTextColor
		duration: Kirigami.Units.shortDuration
		running: false
	}

	// right: icon for muted contact
	// Its size depends on the font's pixel size to be as large as the message counter.
	Kirigami.Icon {
		id: mutedIcon
		source: "audio-volume-muted-symbolic"
		Layout.preferredWidth: Kirigami.Theme.defaultFont.pixelSize * 1.3
		Layout.preferredHeight: Layout.preferredWidth
		visible: notificationsMuted
	}

	// right: icon for pinned chat
	// Its size depends on the font's pixel size to be as large as the message counter.
	Kirigami.Icon {
		source: "window-pin-symbolic"
		Layout.preferredWidth: Kirigami.Theme.defaultFont.pixelSize * 1.3
		Layout.preferredHeight: Layout.preferredWidth
		visible: pinned
	}

	// right: unread message counter
	MessageCounter {
		id: counter
		count: unreadMessages
		muted: notificationsMuted
	}

	// right: icon for reordering
	Kirigami.ListItemDragHandle {
		visible: pinned
		listItem: root
		listView: root.listView
		onMoveRequested: RosterModel.reorderPinnedItem(root.accountJid, root.jid, oldIndex, newIndex)
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
