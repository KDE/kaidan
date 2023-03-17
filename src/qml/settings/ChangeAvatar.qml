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

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import QtQuick.Layouts 1.14
import QtQuick.Dialogs 1.3 as QQD
import QtGraphicalEffects 1.0

import org.kde.kirigami 2.12 as Kirigami
import org.kde.kquickimageeditor 1.0 as KQuickImageEditor

import im.kaidan.kaidan 1.0

import "../elements"

SettingsPageBase {
	id: rootView

	implicitHeight: 600
	implicitWidth: content.implicitWidth

	title: qsTr("Change avatar")
	topPadding: 0

	property string imagePath: Kaidan.avatarStorage.getAvatarUrl(AccountManager.jid)

	Controls.BusyIndicator {
		id: busyIndicator
		visible: false
		anchors.centerIn: parent
		width: 60
		height: 60
	}

	QQD.FileDialog {
		id: fileDialog
		title: qsTr("Choose avatar image")
		folder: shortcuts.home

		selectMultiple: false

		onAccepted: {
			imageDoc.path = fileDialog.fileUrl
			imagePath = fileDialog.fileUrl

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

		KQuickImageEditor.ImageItem {
			id: editImage

			Layout.fillWidth: true
			Layout.fillHeight: true

			Layout.margins: 25

			readonly property real ratioX: editImage.paintedWidth / editImage.nativeWidth;
			readonly property real ratioY: editImage.paintedHeight / editImage.nativeHeight;

			// Assigning this to the contentItem and setting the padding causes weird positioning issues

			fillMode: KQuickImageEditor.ImageItem.PreserveAspectFit
			image: imageDoc.image

			KQuickImageEditor.ImageDocument {
				id: imageDoc
				path: rootView.imagePath
			}

			KQuickImageEditor.SelectionTool {
				id: selectionTool
				width: editImage.paintedWidth
				height: editImage.paintedHeight
				x: editImage.horizontalPadding
				y: editImage.verticalPadding

				KQuickImageEditor.CropBackground {
					anchors.fill: parent
					z: -1
					insideX: selectionTool.selectionX
					insideY: selectionTool.selectionY
					insideWidth: selectionTool.selectionWidth
					insideHeight: selectionTool.selectionHeight
				}
			}
			onImageChanged: {
				selectionTool.selectionX = 0
				selectionTool.selectionY = 0
				selectionTool.selectionWidth = Qt.binding(() => selectionTool.width)
				selectionTool.selectionHeight = Qt.binding(() => selectionTool.height)
			}
		}

		RowLayout {
			Layout.fillWidth: true
			Layout.alignment: Qt.AlignBottom

			Button {
				text: qsTr("Open…")
				Layout.fillWidth: true

				onClicked: {
					fileDialog.open()
				}
			}

			Button {
				text: qsTr("Cancel")
				Layout.fillWidth: true

				onClicked: {
					stack.pop()
				}
			}

			Button {
				text: qsTr("Change")
				Layout.fillWidth: true

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
			// TODO show only if changing didn't succeed
			passiveNotification(qsTr("Avatar changed successfully"))
			stack.pop()
		}
	}
}
