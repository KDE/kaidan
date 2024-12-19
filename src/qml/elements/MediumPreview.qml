// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

ExtendedMessageContent {
	id: root

	property string name
	property string description
	property string size
	property url localFileUrl
	property int type
	property alias previewImage: previewImage
	property alias detailsArea: detailsArea
	property alias detailsTopArea: detailsTopArea
	property real minimumDetailsWidth: minimumWidth - detailsWidthLimitBase
	property real maximumDetailsWidth: maximumWidth - detailsWidthLimitBase
	readonly property real detailsWidthLimitBase: mainArea.leftPadding + previewImage.width + previewImage.Layout.leftMargin + contentArea.spacing + mainArea.rightPadding

	contentArea.data: [
		RoundedImage {
			id: previewImage
			Layout.preferredWidth: Kirigami.Units.iconSizes.huge
			Layout.preferredHeight: Kirigami.Units.iconSizes.huge
			Layout.topMargin: Kirigami.Units.smallSpacing - mainArea.topPadding
			Layout.bottomMargin: Layout.topMargin
			Layout.leftMargin: Layout.topMargin
		},

		ColumnLayout {
			id: detailsArea

			RowLayout {
				id: detailsTopArea

				Controls.Label {
					text: nameTextMetrics.elidedText
					textFormat: Text.PlainText

					TextMetrics {
						id: nameTextMetrics
						text: root.name
						elide: Text.ElideRight
						elideWidth: root.maximumDetailsWidth - detailsTopArea.spacing * 2 - sizeText.implicitWidth
					}
				}

				Item {
					Layout.fillWidth: true
				}

				ScalableText {
					id: sizeText
					text: root.size
					maximumLineCount: 1
					color: Kirigami.Theme.disabledTextColor
					scaleFactor: 0.9
				}
			}
		}
	]

	function open() {
		switch(type) {
		case Enums.MessageUnknown:
		case Enums.MessageFile:
			Qt.openUrlExternally(MediaUtils.localFileDirectoryUrl(localFileUrl))
			break
		default:
			Qt.openUrlExternally(localFileUrl)
		}
	}
}
