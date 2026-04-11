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

	property QtObject selectionModel
	property QtObject modelData

	name: modelData.name
	description: modelData.description
	size: modelData.size
	localFileUrl: modelData.localFileUrl
	type: modelData.type
	minimumDetailsWidth: minimumWidth - detailsWidthLimitBase - root.spacing - mediumRemovalButton.width
	maximumDetailsWidth: maximumWidth - detailsWidthLimitBase - root.spacing - mediumRemovalButton.width
	mainArea.rightPadding: Kirigami.Units.smallSpacing * 3
	previewImage {
		source: modelData.previewImageUrl
		data: OpacityChangingMouseArea {
			opacityItem: parent
			onClicked: {
				switch(type) {
				case Enums.MessageUnknown:
				case Enums.MessageFile:
					MediaUtils.openFileInFolder(localFileUrl)
					break
				default:
					Qt.openUrlExternally(localFileUrl)
				}
			}
		}

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	detailsArea.data: FormattedTextArea {
		text: root.description
		placeholderText: qsTr("Description")
		Layout.minimumWidth: root.minimumDetailsWidth
		Layout.maximumWidth: root.maximumDetailsWidth
		Component.onCompleted: {
			if (!root.description) {
				visible = true
			}
		}
		onTextChanged: modelData.description = text
	}

	IconButton {
		id: mediumRemovalButton
		Controls.ToolTip.text: qsTr("Remove file")
		icon.source: "window-close-symbolic"
		onClicked: root.selectionModel.removeFile(root.modelData.index)
	}
}
