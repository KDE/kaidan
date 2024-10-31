// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Layouts 1.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

MediumPreview {
	id: root

	property var selectionModel
	property var modelData

	name: modelData.name
	description: modelData.description
	size: modelData.size
	localFileUrl: modelData.localFileUrl
	type: modelData.type
	minimumDetailsWidth: minimumWidth - detailsWidthLimitBase - root.spacing - mediumRemovalButton.width
	maximumDetailsWidth: maximumWidth - detailsWidthLimitBase - root.spacing - mediumRemovalButton.width
	mainArea.rightPadding: Kirigami.Units.smallSpacing * 3
	previewImage {
		source: modelData.previewImage
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
