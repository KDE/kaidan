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

/**
 * This element is used in the @see SendMediaSheet to display information about a selected file to
 * the user. It shows the file name, file size and a little file icon.
 */

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtGraphicalEffects 1.14
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0
import MediaUtils 0.1

Rectangle {
	id: root

	required property url mediaSource
	property int messageSize: Kirigami.Units.gridUnit * 14
	required property QtObject message
	required property var file
	required property string messageId

	property bool fileAvailable: file.localFilePath && MediaUtilsInstance.localFileAvailable(file.localFilePath)

	color: "transparent"

	Layout.fillHeight: false
	Layout.fillWidth: true
	Layout.alignment: Qt.AlignLeft
	Layout.topMargin: -6
	Layout.leftMargin: Layout.topMargin
	Layout.rightMargin: Layout.topMargin
	implicitHeight: Kirigami.Units.gridUnit * 3.85
	implicitWidth: layout.implicitWidth + layout.anchors.margins * 2
	Layout.maximumWidth: message ? messageSize : -1

	// content
	ColumnLayout {
		anchors {
			fill: parent
			margins: layout.spacing
		}
		RowLayout {
			id: layout
			spacing: Kirigami.Units.gridUnit * 0.4


			// left: file icon
			Rectangle {
				id: fallbackCircle

				visible: !file.hasThumbnail
				Layout.fillHeight: true
				Layout.preferredWidth: height
				Layout.alignment: Qt.AlignLeft
				radius: height / 2
				color: Qt.lighter(Kirigami.Theme.focusColor, 1.05)

				Kirigami.Icon {
					source: root.fileAvailable ? file.mimeTypeIcon : "download"
					isMask: !openButton.pressed && !openButton.containsMouse
					smooth: true
					height: 24 // we always want the 24x24 icon
					width: height

					anchors {
						centerIn: parent
					}
				}
			}
			Kirigami.Icon {
				id: thumbnailIcon
				visible: file.hasThumbnail
				Layout.fillHeight: true
				Layout.preferredWidth: height
				Layout.alignment: Qt.AlignLeft
				source: file.thumbnailSquare

				layer.enabled: true
				layer.effect: OpacityMask {
					maskSource: Item {
						width: thumbnailIcon.paintedWidth
						height: thumbnailIcon.paintedHeight

						Rectangle {
							anchors.centerIn: parent
							width: Math.min(thumbnailIcon.width, thumbnailIcon.height)
							height: width
							radius: roundedCornersRadius
						}
					}
				}

				Kirigami.Icon {
					source: "download"
					anchors.fill: thumbnailIcon
					visible: !root.fileAvailable
				}
			}

			// right: file description
			ColumnLayout {
				Layout.fillHeight: true
				Layout.fillWidth: true
				spacing: Kirigami.Units.smallSpacing

				// file name
				Controls.Label {
					Layout.fillWidth: true
					text: file.name
					textFormat: Text.PlainText
					elide: Text.ElideRight
					maximumLineCount: 1
				}

				// file size
				Controls.Label {
					Layout.fillWidth: true
					text: Utils.formattedDataSize(file.size)
					textFormat: Text.PlainText
					elide: Text.ElideRight
					maximumLineCount: 1
					color: Kirigami.Theme.disabledTextColor
				}
			}
		}

		// progress bar for upload/download status
		Controls.ProgressBar {
			visible: transferWatcher.isLoading
			value: transferWatcher.progress

			Layout.fillWidth: true
			Layout.maximumWidth: Kirigami.Units.gridUnit * 14
		}

		FileProgressWatcher {
			id: transferWatcher
			fileId: file.fileId
		}
	}

	MouseArea {
		id: openButton
		hoverEnabled: true
		acceptedButtons: Qt.LeftButton | Qt.RightButton

		anchors {
			fill: parent
		}

		onClicked: (event) => {
			if (event.button === Qt.LeftButton) {
				if (root.fileAvailable) {
					Qt.openUrlExternally("file://" + file.localFilePath)
				} else if (file.downloadUrl) {
					Kaidan.fileSharingController.downloadFile(root.messageId, root.file)
				}
			} else if (event.button === Qt.RightButton) {
				root.message.contextMenu.file = root.file
				root.message.contextMenu.message = root.message
				root.message.contextMenu.popup()
			}
		}

		Controls.ToolTip.visible: file.description && openButton.containsMouse
		Controls.ToolTip.delay: Kirigami.Units.longDuration
		Controls.ToolTip.text: file.description
	}
}
