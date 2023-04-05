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
import MediaUtils 0.1

Kirigami.OverlaySheet {
	id: root

	property int sourceType
	property string source
	property MessageComposition composition

	signal rejected()
	signal accepted()

	showCloseButton: false

	contentItem: ColumnLayout {
		// message type preview
		Loader {
			id: loader

			enabled: true
			visible: enabled
			sourceComponent: newMediaComponent

			Layout.fillHeight: item ? item.Layout.fillHeight : false
			Layout.fillWidth: item ? item.Layout.fillWidth : false
			Layout.preferredHeight: item ? item.Layout.preferredHeight : -1
			Layout.preferredWidth: item ? item.Layout.preferredWidth : -1
			Layout.minimumHeight: item ? item.Layout.minimumHeight : -1
			Layout.minimumWidth: item ? item.Layout.minimumWidth : -1
			Layout.maximumHeight: item ? item.Layout.maximumHeight : -1
			Layout.maximumWidth: item ? item.Layout.maximumWidth : -1
			Layout.alignment: item ? item.Layout.alignment : Qt.AlignCenter
			Layout.margins: item ? item.Layout.margins : 0
			Layout.leftMargin: item ? item.Layout.leftMargin : 0
			Layout.topMargin: item ? item.Layout.topMargin : 0
			Layout.rightMargin: item ? item.Layout.rightMargin : 0
			Layout.bottomMargin: item ? item.Layout.bottomMargin : 0

			Component {
				id: newMediaComponent

				NewMediaLoader {
					mediaSourceType: root.sourceType
					mediaSheet: root
				}
			}

			Component {
				id: mediaPreviewComponent

				MediaPreviewLoader {
					mediaSource: root.source
					mediaSourceType: root.sourceType
					mediaSheet: root
				}
			}
		}

		// buttons for send/cancel
		RowLayout {
			Layout.topMargin: Kirigami.Units.largeSpacing
			Layout.fillWidth: true

			Button {
				text: qsTr("Cancel")

				Layout.fillWidth: true

				onClicked: {
					close()
					root.rejected()
				}
			}

			Button {
				id: sendButton

				enabled: root.source
				text: qsTr("Send")

				Layout.fillWidth: true

				onClicked: {
					composition.fileSelectionModel.addFile(root.source)
					composition.send()
					close()
					root.accepted()
				}
			}
		}

		Keys.onPressed: {
			if (event.key === Qt.Key_Return || event.key === Qt.Key_Enter) {
				sendButton.clicked()
			}
		}
	}

	onSheetOpenChanged: {
		if (!sheetOpen) {
			sourceType = Enums.MessageType.MessageUnknown
		}
	}

	function sendMessageType(jid, type) {
		sourceType = type
		open()
	}

	function sendNewMessageType(jid, type) {
		sendMessageType(jid, type)
	}

	function sendFile(jid, url) {
		source = url
		sendMessageType(jid, MediaUtilsInstance.messageType(url))
	}
}
