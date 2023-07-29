// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Layouts 1.14
import QtQuick.Controls 2.14 as Controls
import QtMultimedia 5.14 as Multimedia
import org.kde.kirigami 2.19 as Kirigami
import org.kde.kirigamiaddons.labs.mobileform 0.1 as MobileForm

import im.kaidan.kaidan 1.0

import "../elements"

Controls.Control {
	id: root

	property alias accountJid: fileModel.accountJid
	property alias chatJid: fileModel.chatJid
	property int tabBarCurrentIndex: -1
	property bool selectionMode: false
	readonly property alias totalFilesCount: fileModel.rowCount
	readonly property alias visibleFilesCount: fileProxyModel.rowCount

	leftPadding: 0
	topPadding: 0
	rightPadding: 0
	bottomPadding: 0
	Component.onCompleted: loadDownloadedFiles()
	contentItem: GridView {
		implicitHeight: contentHeight
		boundsMovement: Flickable.StopAtBounds
		cellWidth: {
			switch (root.tabBarCurrentIndex) {
			case 0:
			case 1:
				return root.width / 4
			case 2:
				return width
			}

			return 0
		}
		cellHeight: {
			switch (root.tabBarCurrentIndex) {
			case 0:
			case 1:
				return cellWidth
			case 2:
				return Kirigami.Units.largeSpacing * 8
			}

			return 0
		}
		header: ColumnLayout {
			width: GridView.view.width
			height: implicitHeight
			spacing: 0

			RowLayout {
				id: tabBar
				visible: !root.selectionMode
				spacing: 0

				MobileForm.AbstractFormDelegate {
					id: imagesTab
					checkable: true
					Layout.preferredWidth: tabBar.width / 3
					contentItem: Controls.Label {
						text: qsTr("Images")
						wrapMode: Text.Wrap
						horizontalAlignment: Controls.Label.AlignHCenter
						font.bold: parent.checked
					}
				}

				Kirigami.Separator {
					id: imagesVideosTabSeparator
					Layout.fillHeight: true
				}

				MobileForm.AbstractFormDelegate {
					id: videosTab
					checkable: true
					Layout.preferredWidth: tabBar.width / 3
					contentItem: Controls.Label {
						text: qsTr("Videos")
						wrapMode: Text.Wrap
						horizontalAlignment: Controls.Label.AlignHCenter
						font.bold: parent.checked
					}
				}


				Kirigami.Separator {
					id: videosOtherTabSeparator
					Layout.fillHeight: true
				}

				MobileForm.AbstractFormDelegate {
					id: otherTab
					checkable: true
					Layout.preferredWidth: tabBar.width / 3
					contentItem: Controls.Label {
						text: qsTr("Other")
						wrapMode: Text.Wrap
						horizontalAlignment: Controls.Label.AlignHCenter
						font.bold: parent.checked
					}
				}

				Controls.ButtonGroup {
					id: tabBarGroup
					buttons: [
						imagesTab,
						videosTab,
						otherTab
					]
					onCheckedButtonChanged: {
						switch (checkedButton) {
						case buttons[0]:
							root.tabBarCurrentIndex = 0
							break
						case buttons[1]:
							root.tabBarCurrentIndex = 1
							break
						case buttons[2]:
							root.tabBarCurrentIndex = 2
							break
						default:
							root.tabBarCurrentIndex = -1
							break
						}
					}
				}

				Binding {
					target: tabBarGroup
					property: "checkedButton"
					value: {
						if (root.tabBarCurrentIndex < 0 || root.tabBarCurrentIndex >= tabBarGroup.buttons.length) {
							return null
						}

						return tabBarGroup.buttons[root.tabBarCurrentIndex]
					}
				}
			}

			// tool bar for actions on selected media
			RowLayout {
				visible: root.selectionMode
				Layout.minimumHeight: tabBar.height
				Layout.maximumHeight: Layout.minimumHeight
				Layout.rightMargin: Kirigami.Units.largeSpacing

				MobileForm.AbstractFormDelegate {
					Layout.fillHeight: true
					Layout.fillWidth: false
					contentItem: Kirigami.Icon {
						source: "go-previous-symbolic"
						implicitWidth: Kirigami.Units.iconSizes.small
						implicitHeight: implicitWidth
					}
					onClicked: {
						root.selectionMode = false
						fileProxyModel.clearChecked()
					}
				}

				Controls.Label {
					text: qsTr("%1/%2 selected").arg(fileProxyModel.checkedCount).arg(fileProxyModel.rowCount)
					horizontalAlignment: Qt.AlignLeft
					Layout.fillWidth: true
				}

				Controls.ToolButton {
					visible: fileProxyModel.checkedCount !== fileProxyModel.rowCount
					icon.name: "edit-select-all-symbolic"
					onClicked: {
						fileProxyModel.checkAll()
					}
				}

				Controls.ToolButton {
					icon.name: "edit-delete-symbolic"
					onClicked: {
						fileProxyModel.deleteChecked()
						root.selectionMode = false
					}
				}
			}

			// regular separator
			Kirigami.Separator {
				implicitHeight: Kirigami.Units.smallSpacing
				Layout.fillWidth: true
			}

			// colored marker for current tab selection partly covering the regular separator
			Kirigami.Separator {
				color: Kirigami.Theme.highlightColor
				visible: !root.selectionMode
				implicitHeight: Kirigami.Units.smallSpacing
				Layout.fillWidth: true
				Layout.topMargin: - implicitHeight
				Layout.leftMargin: {
					if (imagesTab.checked) {
						return 0
					}

					if (videosTab.checked) {
						return imagesTab.width
					}

					if (otherTab.checked ) {
						return imagesTab.width + imagesVideosTabSeparator.width + videosTab.width
					}
				}
				Layout.rightMargin: {
					if (otherTab.checked) {
						return 0
					}

					if (videosTab.checked ) {
						return otherTab.width
					}

					if (imagesTab.checked ) {
						return videosTab.width + videosOtherTabSeparator.width + otherTab.width
					}
				}
			}
		}
		model: FileProxyModel {
			id: fileProxyModel
			mode: {
				switch (root.tabBarCurrentIndex) {
				case 0:
					return FileProxyModel.Images
				case 1:
					return FileProxyModel.Videos
				case 2:
					return FileProxyModel.Other
				}

				return FileProxyModel.All
			}
			sourceModel: FileModel {
				id: fileModel
			}
			onFilesDeleted: (files, errors) => {
				if (errors.length > 0) {
					passiveNotification(qsTr("Not all files could be deleted:\n%1").arg(errors[0]))
					console.warn("Not all files could be deleted:", errors)
				}

				root.loadDownloadedFiles()
			}
		}
		delegate: {
			switch (root.tabBarCurrentIndex) {
			case 0:
				return imageDelegate
			case 1:
				return videoDelegate
			case 2:
				return otherDelegate
			}

			return null
		}

		Component {
			id: imageDelegate

			SelectablePreview {
				id: preview
				checkable: root.selectionMode
				checked: checkable && model.checkState === Qt.Checked
				onToggled: {
					model.checkState = checked ? Qt.Checked : Qt.Unchecked
				}
				onClicked: {
					if (root.selectionMode) {
						if (fileProxyModel.checkedCount === 0) {
							root.selectionMode = false
						}
					} else {
						Qt.openUrlExternally(model.file.localFileUrl)
					}
				}
				onPressAndHold: {
					root.selectionMode = true
				}

				Image {
					source: model.file.localFileUrl
					fillMode: Image.PreserveAspectCrop
					asynchronous: true
					sourceSize.width: parent.availableWidth
					sourceSize.height: parent.availableHeight
					anchors.fill: parent

					SelectionMarker {
						visible: preview.containsMouse || checked
						checked: preview.checked
						anchors.top: parent.top
						anchors.right: parent.right
						anchors.topMargin: Kirigami.Units.smallSpacing
						anchors.rightMargin: anchors.topMargin
						onClicked: {
							root.selectionMode = true
							model.checkState = checkState
							preview.toggled()
							preview.clicked()
						}
					}
				}
			}
		}

		Component {
			id: videoDelegate

			SelectablePreview {
				id: preview
				checkable: root.selectionMode
				checked: checkable && model.checkState === Qt.Checked
				onToggled: {
					model.checkState = checked ? Qt.Checked : Qt.Unchecked
				}
				onClicked: {
					if (root.selectionMode) {
						if (fileProxyModel.checkedCount === 0) {
							root.selectionMode = false
						}
					} else {
						Qt.openUrlExternally(model.file.localFileUrl)
					}
				}
				onPressAndHold: {
					root.selectionMode = true
				}

				Multimedia.Video {
					source: model.file.localFileUrl
					autoPlay: true
					fillMode: Multimedia.VideoOutput.PreserveAspectCrop
					anchors.fill: parent

					SelectionMarker {
						visible: preview.containsMouse || checked
						checked: preview.checked
						anchors.top: parent.top
						anchors.right: parent.right
						anchors.topMargin: Kirigami.Units.smallSpacing
						anchors.rightMargin: anchors.topMargin
						onClicked: {
							root.selectionMode = true
							model.checkState = checkState
							preview.toggled()
							preview.clicked()
						}
					}
					onStatusChanged: {
						// Display a thumbnail by playing the first frame and pausing afterwards.
						if (status === Multimedia.MediaPlayer.Buffered) {
							pause()
						}
					}
				}
			}
		}

		Component {
			id: otherDelegate

			Controls.ItemDelegate {
				id: control
				implicitWidth: GridView.view.cellWidth
				implicitHeight: GridView.view.cellHeight
				autoExclusive: false
				checkable: root.selectionMode
				checked: checkable && model.checkState === Qt.Checked
				topPadding: Kirigami.Units.largeSpacing
				bottomPadding: topPadding
				contentItem: MouseArea {
					id: selectionArea
					hoverEnabled: true
					acceptedButtons: Qt.NoButton

					GridLayout {
						anchors.fill: parent

						Kirigami.Icon {
							source: model.file.mimeTypeIcon
							color: Kirigami.Theme.backgroundColor
							Layout.row: 0
							Layout.column: 0
							Layout.rowSpan: 2
							Layout.leftMargin: parent.columnSpacing
							Layout.preferredWidth: parent.height * .8
							Layout.preferredHeight: Layout.preferredWidth
						}

						Controls.Label {
							text: model.file.name
							elide: Qt.ElideRight
							font.bold: true
							Layout.row: 0
							Layout.column: 1
							Layout.fillWidth: true
						}

						Controls.Label {
							text: model.file.details
							Layout.row: 1
							Layout.column: 1
							Layout.fillWidth: true
						}

						SelectionMarker {
							visible: selectionArea.containsMouse || checked
							checked: control.checked
							Layout.row: 0
							Layout.column: 2
							Layout.rowSpan: 2
							Layout.rightMargin: parent.columnSpacing * 2
							onClicked: {
								root.selectionMode = true
								model.checkState = checkState
								control.toggled()
								control.clicked()
							}
						}
					}
				}
				onToggled: {
					model.checkState = checked ? Qt.Checked : Qt.Unchecked
				}
				onClicked: {
					if (root.selectionMode) {
						if (fileProxyModel.checkedCount === 0) {
							root.selectionMode = false
						}
					} else {
						Qt.openUrlExternally(model.file.localFileUrl)
					}
				}
				onPressAndHold: {
					root.selectionMode = true
				}
			}
		}
	}

	function loadFiles() {
		fileModel.loadFiles()
	}

	function loadDownloadedFiles() {
		fileModel.loadDownloadedFiles()
	}
}
