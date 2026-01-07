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
	readonly property bool fileUploadNeeded: message.deliveryState === Enums.DeliveryState.Pending && file.transferOutgoing && file.transferState !== File.TransferState.Done

	name: file.name
	description: file.description
	size: file.formattedSize
	localFileUrl: file.localFileUrl
	type: file.type
	mainArea {
		data: MediumMouseArea {
			id: opacityChangingMouseArea
			opacityItem: parent.background
			selected: root.message.contextMenu && root.message.contextMenu.file === file
			acceptedButtons: Qt.LeftButton | Qt.RightButton
			onClicked: (event) => {
				if (event.button === Qt.LeftButton) {
					if (root.file.transferState === File.TransferState.Pending || root.file.transferState === File.TransferState.Transferring) {
						root.message.chatController.account.fileSharingController.cancelTransfer(root.message.chatController.jid, root.message.msgId, root.file)
					} else if (!root.file.locallyAvailable) {
						root.message.chatController.account.fileSharingController.downloadFile(root.message.chatController.jid, root.message.msgId, root.file)
					} else if (root.fileUploadNeeded) {
						root.message.chatController.account.fileSharingController.sendFile(root.message.chatController.jid, root.message.msgId, root.file, message.encryption !== Encryption.NoEncryption)
					} else if (root.file.locallyAvailable) {
						root.open()
					}
				} else if (event.button === Qt.RightButton) {
				   if (root.file.locallyAvailable) {
					   root.message.showContextMenu(this, root.file)
				   } else {
					   root.message.showContextMenu(this)
				   }
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
		readonly property bool iconShown: root.file.transferState === File.TransferState.Transferring || !root.file.locallyAvailable || root.fileUploadNeeded

		opacity: iconShown ? 1 : mainAreaBackground.opacity
		source: ImageProvider.generatedFileImageUrl(file)
		data: CircleProgressBar {
			value: transferWatcher.progress
			opacity: iconShown ? (opacityChangingMouseArea.containsMouse ?  0.8 : 0.5) : 0
			// Do not apply the opacity to child items.
			layer.enabled: true
			anchors.fill: parent
			anchors.margins: Kirigami.Units.smallSpacing

			Behavior on opacity {
				NumberAnimation {}
			}

			Kirigami.Icon {
				source: {
					if (root.file.transferState === File.TransferState.Pending || root.file.transferState === File.TransferState.Transferring) {
						return "content-loading-symbolic"
					}

					if (root.fileUploadNeeded) {
						return "view-refresh-symbolic"
					}

					if (!root.file.locallyAvailable) {
						return "folder-download-symbolic"
					}
				}
				color: root.file.transferState === File.TransferState.Transferring ? Kirigami.Theme.activeTextColor : Kirigami.Theme.textColor
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
