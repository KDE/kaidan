// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Controls.ItemDelegate {
	id: root

	property var iconSource
	property double iconSize: Kirigami.Units.iconSizes.medium
	property color iconColor: Kirigami.Theme.textColor

	background: Kirigami.ShadowedRectangle {
		color: secondaryBackgroundColor
		opacity: {
			if (parent.pressed) {
				return 1
			}

			if (parent.hovered) {
				return 0.9
			}

			return 0.8
		}
		radius: height / 2
		shadow.color: Qt.darker(color, 1.2)
		shadow.size: 4

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	contentItem: Kirigami.Icon {
		source: root.iconSource
		color: root.iconColor
		isMask: true
		implicitWidth: root.iconSize
		implicitHeight: root.iconSize
	}
	horizontalPadding: iconSize * 0.7
	verticalPadding: horizontalPadding
}
