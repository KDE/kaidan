// SPDX-FileCopyrightText: 2018 Linus Jahn <lnj@kaidan.im>
// SPDX-FileCopyrightText: 2019 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2019 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2022 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

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
