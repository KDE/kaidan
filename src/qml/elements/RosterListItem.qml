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
import QtGraphicalEffects 1.14
import org.kde.kirigami 2.12 as Kirigami

import im.kaidan.kaidan 1.0

UserListItem {
	id: root

	property string lastMessage
	property int unreadMessages
	property bool pinned
	property Controls.Menu contextMenu

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
			text: name
			textFormat: Text.PlainText
			elide: Text.ElideRight
			maximumLineCount: 1
			level: 3
			Layout.fillWidth: true
			Layout.maximumHeight: Kirigami.Units.gridUnit * 1.5
		}

		// last message or error status message if available, otherwise not visible
		Controls.Label {
			visible: text
			Layout.fillWidth: true
			elide: Text.ElideRight
			maximumLineCount: 1
			text: Utils.removeNewLinesFromString(lastMessage)
			textFormat: Text.PlainText
			font.weight: Font.Light
		}
	}

	// right: icon for muted contact
	Kirigami.Icon {
		id: mutedIcon
		source: "audio-volume-muted-symbolic"
		width: 22
		height: 22
		visible: mutedWatcher.muted
	}

	NotificationsMutedWatcher {
		id: mutedWatcher
		jid: root.jid
	}

	// right: icon for pinned chat
	Kirigami.Icon {
		source: "window-pin-symbolic"
		width: 16
		height: 16
		visible: pinned
	}

	// right: unread message counter
	MessageCounter {
		id: counter
		count: unreadMessages
		muted: mutedWatcher.muted
		Layout.preferredHeight: Kirigami.Units.gridUnit * 1.25
		Layout.preferredWidth: Kirigami.Units.gridUnit * 1.25
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
