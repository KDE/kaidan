// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

MediumPreview {
	id: root

	property var file
	property Item message

	name: file.name
	description: file.description
	size: file.formattedSize
	localFileUrl: file.localFileUrl
	type: file.type
	mainArea {
		data: OpacityChangingMouseArea {
			id: opacityChangingMouseArea
			opacityItem: parent.background
			acceptedButtons: Qt.LeftButton | Qt.RightButton
			onClicked: (event) => {
				if (event.button === Qt.LeftButton) {
					if (root.localFileUrl.toString()) {
						root.open()
					} else if (!transferWatcher.isLoading) {
						root.message.chatController.account.fileSharingController.downloadFile(root.message.chatController.jid, root.message.msgId, root.file)
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

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	mainAreaBackground {
		Behavior on opacity {
			NumberAnimation {}
		}
	}
	previewImage {
		opacity: (transferWatcher.isLoading || !root.file.localFileUrl.toString()) ? 1 : mainAreaBackground.opacity
		source: {
			const image = file.previewImage

			if (MediaUtils.imageValid(image)) {
				return image
			}

			return file.mimeTypeIcon
		}
		data: CircleProgressBar {
			value: transferWatcher.progress
			opacity: (transferWatcher.isLoading || !root.file.localFileUrl.toString()) ? (opacityChangingMouseArea.containsMouse ?  0.8 : 0.5 ): 0
			// Do not apply the opacity to child items.
			layer.enabled: true
			anchors.fill: parent
			anchors.margins: Kirigami.Units.smallSpacing

			Behavior on opacity {
				NumberAnimation {}
			}

			Kirigami.Icon {
				source: root.file.localFileUrl.toString() ? "content-loading-symbolic" : "folder-download-symbolic"
				color: transferWatcher.isLoading ? Kirigami.Theme.activeTextColor : Kirigami.Theme.textColor
				isMask: true
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
