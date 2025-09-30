// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

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
			onClicked: root.open()
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

	ClickableIcon {
		id: mediumRemovalButton
		Controls.ToolTip.text: qsTr("Remove file")
		source: "window-close-symbolic"
		Layout.preferredHeight: Kirigami.Units.iconSizes.smallMedium
		onClicked: root.selectionModel.removeFile(root.modelData.index)
	}
}
