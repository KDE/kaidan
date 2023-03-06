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

UserListItem {
	id: root

	property ListView listView
	property Controls.Menu contextMenu
	property bool lastMessageIsDraft
	property string lastMessage
	property int unreadMessages
	property bool pinned

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
		visible: mutedWatcher.muted
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
		muted: mutedWatcher.muted
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

	NotificationsMutedWatcher {
		id: mutedWatcher
		jid: root.jid
	}

	function showContextMenu() {
		if (contextMenu) {
			contextMenu.item = this
			contextMenu.popup()
		}
	}
}
