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
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import org.kde.kirigami 2.14 as Kirigami

import im.kaidan.kaidan 1.0

Kirigami.OverlaySheet {
	id: root

	property var chatSupportList
	property bool isGroupChatSupportSheet: false

	header: Kirigami.Heading {
		text: isGroupChatSupportSheet ? qsTr("Support Group") : qsTr("Support")

		wrapMode: Text.WordWrap
	}

	ListView {
		implicitWidth: largeButtonWidth

		clip: true
		model: Array.from(chatSupportList)

		Controls.ScrollBar.vertical: Controls.ScrollBar {
		}

		RosterAddContactSheet {
			id: addContactSheet
		}

		delegate: Kirigami.AbstractListItem {
			required property int index
			required property string modelData
			readonly property string chatName: {
				(isGroupChatSupportSheet ? qsTr("Group Support %1") : qsTr("Support %1")).arg(index + 1)
			}

			height: 65

			ColumnLayout {
				spacing: 12

				Controls.Label {
					text: chatName
					font.bold: true
					Layout.fillWidth: true
				}

				Controls.Label {
					text: modelData
					wrapMode: Text.Wrap
					Layout.fillWidth: true
				}
			}

			onClicked: {
				if (isGroupChatSupportSheet) {
					Qt.openUrlExternally("xmpp:" + modelData + "?join")
				} else {
					if (!addContactSheet.sheetOpen) {
						addContactSheet.jid = modelData
						addContactSheet.nickName = chatName
						addContactSheet.open()
					}
				}

				root.close()
			}
		}
	}
}
