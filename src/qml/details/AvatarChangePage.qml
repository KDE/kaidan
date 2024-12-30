// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtCore
import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Dialogs as Dialogs
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
// import org.kde.kquickimageeditor as KQuickImageEditor

import im.kaidan.kaidan

import "../elements"

Kirigami.Page {
	id: root

	property string imagePath: Kaidan.avatarStorage.getAvatarUrl(AccountManager.jid)

	title: qsTr("Change profile image")
	Component.onDestruction: openView(accountDetailsDialog, accountDetailsPage).jid = AccountManager.jid

	Controls.BusyIndicator {
		id: busyIndicator
		visible: false
		anchors.centerIn: parent
		width: 60
		height: 60
	}

	Dialogs.FileDialog {
		id: fileDialog
		title: qsTr("Choose profile image")
		currentFolder: StandardPaths.standardLocations(StandardPaths.HomeLocation)[0]
		fileMode: Dialogs.FileDialog.OpenFile
		onAccepted: {
			imageDoc.path = fileDialog.selectedFile
			imagePath = fileDialog.selectedFile

			fileDialog.close()
		}
		onRejected: {
			fileDialog.close()
		}
		Component.onCompleted: {
			visible = false
		}
	}

	ColumnLayout {
		id: content
		visible: !busyIndicator.visible
		spacing: 0
		anchors.fill: parent

		// KQuickImageEditor.ImageItem {
		// 	id: editImage

		// 	Layout.fillWidth: true
		// 	Layout.fillHeight: true

		// 	Layout.margins: 25

		// 	readonly property real ratioX: editImage.paintedWidth / editImage.nativeWidth;
		// 	readonly property real ratioY: editImage.paintedHeight / editImage.nativeHeight;

		// 	// Assigning this to the contentItem and setting the padding causes weird positioning issues

		// 	fillMode: KQuickImageEditor.ImageItem.PreserveAspectFit
		// 	image: imageDoc.image

		// 	KQuickImageEditor.ImageDocument {
		// 		id: imageDoc
		// 		path: root.imagePath
		// 	}

		// 	KQuickImageEditor.SelectionTool {
		// 		id: selectionTool
		// 		width: editImage.paintedWidth
		// 		height: editImage.paintedHeight
		// 		x: editImage.horizontalPadding
		// 		y: editImage.verticalPadding

		// 		KQuickImageEditor.CropBackground {
		// 			anchors.fill: parent
		// 			z: -1
		// 			insideX: selectionTool.selectionX
		// 			insideY: selectionTool.selectionY
		// 			insideWidth: selectionTool.selectionWidth
		// 			insideHeight: selectionTool.selectionHeight
		// 		}
		// 	}
		// 	onImageChanged: {
		// 		selectionTool.selectionX = 0
		// 		selectionTool.selectionY = 0
		// 		selectionTool.selectionWidth = Qt.binding(() => selectionTool.width)
		// 		selectionTool.selectionHeight = Qt.binding(() => selectionTool.height)
		// 	}
		// }

		ColumnLayout {
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth

			CenteredAdaptiveButton {
				text: qsTr("Open image…")
				onClicked: fileDialog.open()
			}

			CenteredAdaptiveButton {
				text: qsTr("Remove current profile image")
				visible: root.imagePath
				onClicked: Kaidan.client.vCardManager.changeAvatarRequested()
			}

			CenteredAdaptiveHighlightedButton {
				text: qsTr("Save selection")
				visible: root.imagePath
				onClicked: {
					imageDoc.crop(
						selectionTool.selectionX / editImage.ratioX,
						selectionTool.selectionY / editImage.ratioY,
						selectionTool.selectionWidth / editImage.ratioX,
						selectionTool.selectionHeight / editImage.ratioY
					)

					Kaidan.client.vCardManager.changeAvatarRequested(imageDoc.image)
					busyIndicator.visible = true
				}
			}
		}
	}

	Connections {
		target: Kaidan

		function onAvatarChangeSucceeded() {
			busyIndicator.visible = false
			// TODO: Show error message if changing did not succeed
			popLayer()
		}
	}
}
