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
		data: MediumMouseArea {
			id: opacityChangingMouseArea
			opacityItem: parent.background
			selected: root.message.contextMenu?.file === file
			acceptedButtons: Qt.LeftButton | Qt.RightButton
			onClicked: (event) => {
				if (event.button === Qt.LeftButton) {
					if (root.file.transferState === File.TransferState.Pending || root.file.transferState === File.TransferState.Transferring) {
						root.message.chatController.account.fileSharingController.cancelTransfer(root.message.chatController.jid, root.message.msgId, root.file)
					} else if (!root.file.locallyAvailable) {
						root.message.chatController.account.fileSharingController.downloadFile(root.message.chatController.jid, root.message.msgId, root.file)
					} else if (transferProgressBar.fileUploadNeeded) {
						root.message.chatController.account.fileSharingController.sendFile(root.message.chatController.jid, root.message.msgId, root.file, message.encryption !== Encryption.NoEncryption)
					} else if (root.file.locallyAvailable) {
						root.open()
					}
				} else if (event.button === Qt.RightButton) {
					root.message.showContextMenu(this, root.file)
				}
			}
		}
	}
	mainAreaBackground {
		Behavior on opacity {
			NumberAnimation {}
		}
	}
	previewImage {
		opacity: transferProgressBar.opacity ? 1 : mainAreaBackground.opacity
		source: ImageProvider.generatedFileImageUrl(file)
		blurEnabled: !root.file.locallyAvailable && root.file.hasThumbnail
		blurMax: 8
	}
	previewImageArea {
		data: MediumTransferProgressBar {
			id: transferProgressBar
			file: root.file
			deliveryState: root.message.deliveryState
			opacity: shown ? (opacityChangingMouseArea.containsMouse || opacityChangingMouseArea.selected ? 0.8 : 0.5) : 0
			anchors.fill: parent
			anchors.margins: Kirigami.Units.smallSpacing
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
}
