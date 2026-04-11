// SPDX-FileCopyrightText: 2026 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtQuick.Controls as Controls
import QtMultimedia as Multimedia
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

import im.kaidan.kaidan

Controls.Control {
	id: root

	property ChatController chatController
	property string fileId

	signal highlightMessageRequested(messageId: string)

	topPadding: 0
	bottomPadding: 0
	leftPadding: 0
	rightPadding: 0
	topInset: 0
	bottomInset: 0
	leftInset: 0
	rightInset: 0
	hoverEnabled: true
	contentItem: ListView {
		id: mediaListView

		readonly property QtObject currentItemModel: currentItem?.model ?? null
		readonly property bool currentItemLocallyAvailable: currentItemModel?.file.locallyAvailable ?? false

		orientation: ListView.Horizontal
		highlightRangeMode: ListView.StrictlyEnforceRange
		snapMode: ListView.SnapOneItem
		clip: true
		highlightMoveDuration: Kirigami.Units.longDuration
		model: FileProxyModel {
			id: fileProxyModel
			mode: FileProxyModel.All
			locallyAvailableOnly: false
			sourceModel: FileModel {
				id: fileModel
				accountJid: root.chatController.account.settings.jid
				chatJid: root.chatController.jid
				onRowCountChanged: mediaListView.currentIndex = Math.max(0, fileProxyModel.fileIndex(root.fileId).row)
			}
		}
		delegate: Loader {
			required property QtObject model
			// Used to get x of the item within mediaListView.
			readonly property real contentItemX: x
			// Used to fade out the item while being flicked.
			readonly property real contentItemOpacity: {
				const opacityChangeIntensity = 0.4

				const itemCenterX = x + width / 2
				const contentCenterX = ListView.view.contentX + Math.round(width / 2)
				const centerDistance = Math.abs(contentCenterX - itemCenterX)
				const relativeCenterDistance = centerDistance / width * 2

				return (1 - (relativeCenterDistance * opacityChangeIntensity))
			}

			width: ListView.view.width
			height: ListView.view.height
			sourceComponent: {
				const file = model.file
				return file.type === Enums.MessageType.MessageVideo && file.locallyAvailable ? videoDelegate : imageDelegate
			}
		}
		Layout.fillWidth: true
		Layout.fillHeight: true

		IconButtonArea {
			opacity: Kirigami.Settings.isMobile
			visible: opacity
			contentItem: RowLayout {
				spacing: Kirigami.Units.smallSpacing

				IconButton {
					Controls.ToolTip.text: qsTr("Close media viewer")
					icon.source: "go-previous-symbolic"
					onClicked: popPage()
				}
			}
			anchors {
				top: parent.top
				left: parent.left
				topMargin: Kirigami.Units.largeSpacing
				leftMargin: Kirigami.Units.largeSpacing
			}
		}

		IconButtonArea {
			id: toolbar
			opacity: root.hovered || Kirigami.Settings.isMobile
			contentItem: RowLayout {
				spacing: Kirigami.Units.smallSpacing

				IconButton {
					id: detailsButton
					Controls.ToolTip.text: checked ? qsTr("Hide details") : qsTr("Show details")
					icon.source: "info-symbolic"
					checkable: true
				}

				IconButton {
					Controls.ToolTip.text: qsTr("Go to message")
					icon.source: "internet-mail-symbolic"
					visible: mediaListView.currentItemLocallyAvailable
					onClicked: root.highlightMessageRequested(mediaListView.currentItemModel.message.relevantId)
				}

				IconButton {
					Controls.ToolTip.text: qsTr("Remove")
					icon.source: "edit-delete-symbolic"
					visible: mediaListView.currentItemModel?.file.locallyAvailable ?? false
					onClicked: {
						const model = mediaListView.currentItemModel
						root.chatController.messageModel.deleteFile(model.message.relevantId, model.file)
					}
				}

				IconButton {
					Controls.ToolTip.text: qsTr("Open in folder")
					icon.source: "folder-symbolic"
					visible: mediaListView.currentItemLocallyAvailable
					onClicked: MediaUtils.openFileInFolder(mediaListView.currentItemModel.file.localFileUrl)
				}

				IconButton {
					Controls.ToolTip.text: qsTr("Open externally")
					icon.source: "view-fullscreen-symbolic"
					visible: mediaListView.currentItemLocallyAvailable
					onClicked: {
						if (mediaPlayerControl.player.playing) {
							mediaPlayerControl.player.pause()
						}

						Qt.openUrlExternally(mediaListView.currentItemModel.file.localFileUrl)
					}
				}
			}
			anchors {
				top: parent.top
				right: parent.right
				topMargin: Kirigami.Units.largeSpacing
				rightMargin: Kirigami.Units.largeSpacing
			}

			Behavior on implicitWidth {
				SmoothedAnimation {
					duration: mediaListView.highlightMoveDuration
				}
			}
		}

		Controls.Control {
			id: detailsArea
			opacity: detailsButton.checked ? toolbar.mainBackground.opacity : 0
			contentItem: ColumnLayout {
				FormCard.FormCard {
					clip: true
					visible: implicitHeight > 2
					maximumWidth: parent.width - Kirigami.Units.smallSpacing

					Behavior on implicitHeight {
						SmoothedAnimation {
							duration: mediaListView.highlightMoveDuration
						}
					}

					FormCard.FormTextDelegate {
						text: qsTr("Message")
						description: mediaListView.currentItemModel?.message?.body ?? ""
						visible: description.length
						Layout.alignment: Qt.AlignTop
					}

					FormCard.FormTextDelegate {
						text: qsTr("Description")
						description: mediaListView.currentItemModel?.file?.description ?? ""
						visible: description.length
					}
				}

				FormCard.FormCard {
					clip: true
					visible: implicitHeight > 2
					maximumWidth: parent.width - Kirigami.Units.smallSpacing

					Behavior on implicitHeight {
						SmoothedAnimation {
							duration: mediaListView.highlightMoveDuration
						}
					}

					FormCard.FormTextDelegate {
						text: qsTr("Name")
						description: mediaListView.currentItemModel?.file?.name ?? ""
						Layout.alignment: Qt.AlignTop
					}
				}

				FormCard.FormCard {
					clip: true
					visible: implicitHeight > 2
					maximumWidth: parent.width - Kirigami.Units.smallSpacing

					Behavior on implicitHeight {
						SmoothedAnimation {
							duration: mediaListView.highlightMoveDuration
						}
					}

					FormCard.FormTextDelegate {
						text: qsTr("Size")
						description: mediaListView.currentItemModel?.file?.formattedSize ?? ""
						visible: description.length
						Layout.alignment: Qt.AlignTop
					}

					FormCard.FormTextDelegate {
						text: qsTr("Sent/Received")
						description: mediaListView.currentItemModel?.message?.formattedTimestamp ?? ""
						visible: description.length
					}
				}
			}
			anchors {
				top: toolbar.bottom
				right: toolbar.right
				topMargin: toolbar.anchors.topMargin
			}
			width: Kirigami.Units.gridUnit * 14
			topPadding: 0
			bottomPadding: 0
			leftPadding: 0
			rightPadding: 0

			Behavior on opacity {
				NumberAnimation {}
			}
		}

		IconButtonArea {
			id: navigationArea
			opacity: !Kirigami.Settings.isMobile && root.hovered
			visible: opacity
			contentItem: RowLayout {
				spacing: Kirigami.Units.smallSpacing

				IconButton {
					Controls.ToolTip.text: qsTr("Previous")
					icon.source: "go-previous-symbolic"
					enabled: mediaListView.currentIndex > 0
					onClicked: mediaListView.decrementCurrentIndex()
				}

				IconButton {
					Controls.ToolTip.text: qsTr("Next")
					icon.source: "go-next-symbolic"
					enabled: mediaListView.currentIndex < mediaListView.count - 1
					onClicked: mediaListView.incrementCurrentIndex()
				}
			}
			anchors {
				left: parent.left
				bottom: parent.bottom
				leftMargin: Kirigami.Units.largeSpacing
				bottomMargin: Kirigami.Units.largeSpacing
			}
		}

		MediaPlayerControl {
			id: mediaPlayerControl
			opacity: {
				const file = mediaListView.currentItemModel?.file

				return (root.hovered || Kirigami.Settings.isMobile) && file && file.localFileUrl.toString() && (file.type === Enums.MessageType.MessageAudio || file.type === Enums.MessageType.MessageVideo) ? 1 : 0
			}
			visible: opacity
			player {
				source: {
					const file = mediaListView.currentItemModel?.file

					if (file && (file.type === Enums.MessageType.MessageAudio || file.type === Enums.MessageType.MessageVideo)){
						return file.localFileUrl
					}

					return ""
				}
				onMediaStatusChanged: {
					// This is needed to play the medium once it is loaded since "autoPlay" seems to not work for large audio files.
					if (player.mediaStatus === Multimedia.MediaPlayer.LoadedMedia) {
						player.play()
					}
				}
			}
			anchors {
				left: Kirigami.Settings.isMobile ? parent.left : navigationArea.right
				verticalCenter: navigationArea.verticalCenter
				right: parent.right
				leftMargin: Kirigami.Units.largeSpacing
				rightMargin: Kirigami.Units.largeSpacing
			}
			Layout.fillWidth: true
			Layout.bottomMargin: detailsArea.height + detailsArea.spacing
		}
	}
	Component.onCompleted: fileModel.loadFiles()

	Shortcut {
		sequence: "Left"
		onActivated: mediaListView.decrementCurrentIndex()
	}

	Shortcut {
		sequence: "Right"
		onActivated: mediaListView.incrementCurrentIndex()
	}

	MouseArea {
		acceptedButtons: Qt.BackButton | Qt.ForwardButton
		anchors.fill: parent
		onClicked: event => {
			if (event.button === Qt.BackButton) {
				mediaListView.decrementCurrentIndex()
			} else if (event.button === Qt.ForwardButton) {
				mediaListView.incrementCurrentIndex()
			}
		}
	}

	Component {
		id: imageDelegate

		Item {
			anchors.fill: parent

			RoundedImage {
				id: image

				readonly property bool iconForLocallyUnavailableFileShown: {
					const file = model?.file
					return file ? !file.locallyAvailable && !file.hasThumbnail : false
				}

				readonly property bool iconShown: {
					const file = model?.file
					return file ? (file.type !== Enums.MessageType.MessageImage && file.type !== Enums.MessageType.MessageVideo) || iconForLocallyUnavailableFileShown : false
				}

				source: {
					const file = model?.file
					return file ? ImageProvider.generatedFileImageUrl(file) : ""
				}
				radius: Kirigami.Units.cornerRadius
				fillMode: Image.PreserveAspectFit
				blurEnabled: {
					const file = model?.file
					return file ? !file.locallyAvailable && file.hasThumbnail : false
				}
				horizontalAlignment: Image.AlignHCenter
				verticalAlignment: Image.AlignVCenter
				width: iconShown ? Kirigami.Units.iconSizes.enormous * 2 : parent.width
				height: iconShown ? Kirigami.Units.iconSizes.enormous * 2 : parent.height
				anchors.centerIn: parent

				Behavior on opacity {
					NumberAnimation {}
				}

				// Fades the item out while being flicked.
				MultiEffect {
					colorization: 1 - contentItemOpacity
					colorizationColor: parent.iconShown ? Kirigami.Theme.backgroundColor : "black"
					source: parent
					anchors.fill: parent
				}
			}

			Loader {
				sourceComponent: model?.file && model?.message ? transferProgressBar : undefined
				anchors.centerIn: parent

				Component {
					id: transferProgressBar

					MediumTransferProgressBar {
						file: model.file
						deliveryState: model.message.deliveryState
						opacity: {
							if ((root.hovered || Kirigami.Settings.isMobile) && shown) {
								let itemOpacity

								if (opacityChangingMouseArea.pressed) {
									itemOpacity = 0.7
								} else if (opacityChangingMouseArea.containsMouse) {
									itemOpacity = 0.55
								} else {
									itemOpacity = 0.35
								}

								return image.iconForLocallyUnavailableFileShown ? itemOpacity + 0.2 : itemOpacity
							}

							return 0
						}
						visible: opacity

						OpacityChangingMouseArea {
							id: opacityChangingMouseArea
							opacityItem: image
							baseOpacity: image.iconForLocallyUnavailableFileShown ? 0.15 : 1
							hoverOpacity: image.iconForLocallyUnavailableFileShown ? 0.1 : 0.9
							enabled: parent.visible
							onClicked: {
								const chatJid = root.chatController.jid
								const file = model.file
								const message = model.message
								const messageId = message.relevantId

								if (file.transferState === File.TransferState.Pending || file.transferState === File.TransferState.Transferring) {
									root.chatController.account.fileSharingController.cancelTransfer(chatJid, messageId, file)
								} else if (!file.locallyAvailable) {
									root.chatController.account.fileSharingController.downloadFile(chatJid, messageId, file)
								} else {
									root.chatController.account.fileSharingController.sendFile(chatJid, messageId, file, message.encryption !== Encryption.NoEncryption)
								}
							}
						}
					}
				}
			}
		}
	}

	Component {
		id: videoDelegate

		Item {
			id: videoArea

			Multimedia.VideoOutput {
				id: videoOutput
				anchors.fill: parent
				endOfStreamPolicy: Multimedia.VideoOutput.KeepLastFrame

				// Fades the item out while being flicked.
				MultiEffect {
					colorization: 1 - contentItemOpacity
					colorizationColor: "black"
					source: parent
					anchors.fill: parent
				}

				Multimedia.MediaPlayer {
					id: firstLastFramePlayer
					source: model?.file?.localFileUrl ?? ""
					videoOutput: videoOutput
					autoPlay: true
					onPlayingChanged: {
						position = 0
						pause()
					}
				}

				Connections {
					target: mediaListView

					function onContentXChanged() {
						// Reset the preview once the item is out of view.
						// That way, a new preview is displayed when flicking again to the item instead of displaying the last position.
						if (contentItemX - videoArea.width >= mediaListView.contentX || contentItemX + videoArea.width <= mediaListView.contentX) {
							firstLastFramePlayer.videoOutput = videoOutput
							firstLastFramePlayer.play()
						}
					}

					function onCurrentItemChanged() {
						if (mediaListView.currentItem.item === videoArea) {
							mediaPlayerControl.player.videoOutput = videoOutput
						} else if (mediaPlayerControl.player.videoOutput === videoOutput) {
							mediaPlayerControl.player.videoOutput = null
						}
					}
				}
			}
		}
	}
}
