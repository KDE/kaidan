// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

MediumPreview {
	id: root

	property var file
	property Item message

	name: file.name
	description: file.description
	size: file.formattedSize
	localFileUrl: file.localFileUrl
	type: file.type
	mainArea.data: OpacityChangingMouseArea {
		opacityItem: parent.background
		acceptedButtons: Qt.LeftButton | Qt.RightButton
		onClicked: (event) => {
			if (event.button === Qt.LeftButton) {
				if (root.localFileUrl.toString()) {
					root.open()
				} else if (!transferWatcher.isLoading) {
					Kaidan.fileSharingController.downloadFile(root.message.msgId, root.file)
				}
			} else if (event.button === Qt.RightButton) {
			   if (root.localFileUrl.toString()) {
				   root.message.showContextMenu(this, root.file)
			   } else {
				   root.message.showContextMenu(this)
			   }
			}
		}
	}
	mainAreaBackground {
		data: Controls.ProgressBar {
			id: transferProgressBar
			value: transferWatcher.progress
			visible: value
			opacity: 0.3
			background: null
			contentItem: Rectangle {
				width: transferProgressBar.visualPosition * parent.width
				height: parent.height
				color: Kirigami.Theme.activeTextColor
				radius: root.mainAreaBackground.radius
			}
			anchors.fill: parent
		}

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	previewImage {
		source: {
			const image = file.previewImage

			if (MediaUtils.imageValid(image)) {
				return image
			}

			return file.mimeTypeIcon
		}
		data: Rectangle {
			anchors.fill: parent
			color: Kirigami.Theme.backgroundColor
			visible: !root.file.localFileUrl.toString()
			opacity: mainAreaBackground.opacity < 1 ? 0.8 : 0.5
			// Do not apply the opacity to child items.
			layer.enabled: true

			Behavior on opacity {
				NumberAnimation {}
			}

			Kirigami.Icon {
				source: "folder-download-symbolic"
				color: Kirigami.Theme.textColor
				width: Kirigami.Units.iconSizes.medium
				height: Kirigami.Units.iconSizes.medium
				anchors.centerIn: parent
			}
		}
	}
	detailsArea {
		data: Loader {
			id: detailsLoader
			sourceComponent: root.description ? descriptionArea : undefined
			Layout.minimumWidth: root.minimumDetailsWidth
			Layout.maximumWidth: root.maximumDetailsWidth

			Component {
				id: descriptionArea

				FormattedTextEdit {
					text: root.description
					color: Kirigami.Theme.disabledTextColor
				}
			}
		}
	}

	FileProgressWatcher {
		id: transferWatcher
		fileId: root.file.fileId
	}
}
