// SPDX-FileCopyrightText: 2023 Tibor Csötönyi <work@taibsu.de>
// SPDX-FileCopyrightText: 2023 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2023 Jonah Brüchert <jbb@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kquickimageeditor as KQuickImageEditor

import im.kaidan.kaidan

import "../elements"

Kirigami.Page {
	id: root

	property url imagePath: Kaidan.avatarStorage.getAvatarUrl(AccountController.account.jid)

	title: qsTr("Change profile image")
	Component.onDestruction: openView(accountDetailsDialog, accountDetailsPage).jid = AccountController.account.jid

	Controls.BusyIndicator {
		id: busyIndicator
		visible: false
		anchors.centerIn: parent
		width: 60
		height: 60
	}

	ColumnLayout {
		id: content
		visible: !busyIndicator.visible
		spacing: 0
		anchors.fill: parent

		KQuickImageEditor.ImageItem {
			id: editImage

			readonly property real ratioX: editImage.paintedWidth / editImage.nativeWidth
			readonly property real ratioY: editImage.paintedHeight / editImage.nativeHeight

			Layout.fillWidth: true
			Layout.fillHeight: true
			Layout.margins: 25

			// Assigning this to the contentItem and setting the padding causes weird positioning issues

			fillMode: KQuickImageEditor.ImageItem.PreserveAspectFit
			image: imageDoc.image
			onImageChanged: {
				selectionTool.selectionX = 0
				selectionTool.selectionY = 0
				selectionTool.selectionWidth = Qt.binding(() => selectionTool.width)
				selectionTool.selectionHeight = Qt.binding(() => selectionTool.height)
			}

			KQuickImageEditor.ImageDocument {
				id: imageDoc
				path: root.imagePath
			}

			KQuickImageEditor.SelectionTool {
				id: selectionTool
				width: editImage.paintedWidth
				height: editImage.paintedHeight
				x: editImage.horizontalPadding
				y: editImage.verticalPadding
				aspectRatio: KQuickImageEditor.SelectionTool.Square

				KQuickImageEditor.CropBackground {
					anchors.fill: parent
					z: -1
					insideX: selectionTool.selectionX
					insideY: selectionTool.selectionY
					insideWidth: selectionTool.selectionWidth
					insideHeight: selectionTool.selectionHeight
				}
			}
		}

		ColumnLayout {
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter
			Layout.maximumWidth: largeButtonWidth

			CenteredAdaptiveButton {
				text: qsTr("Open image…")
				onClicked: root.imagePath = MediaUtils.openFile()
			}

			CenteredAdaptiveButton {
				text: qsTr("Remove current profile image")
				visible: root.imagePath.toString()
				onClicked: Kaidan.client.vCardManager.changeAvatarRequested()
			}

			CenteredAdaptiveHighlightedButton {
				text: qsTr("Save selection")
				visible: root.imagePath.toString()
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
