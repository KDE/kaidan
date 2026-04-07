// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Controls.ItemDelegate {
	id: root

	property color backgroundColor: secondaryBackgroundColor
	property bool strongBackgroundOpacityChange: false
	property var iconSource
	property double iconSize: Kirigami.Units.iconSizes.small
	property alias iconColor: icon.color

	background: Kirigami.ShadowedRectangle {
		color: root.backgroundColor
		opacity: {
			let defaultOpacity = root.strongBackgroundOpacityChange ? 0 : 0.8

			if (!parent.enabled) {
				return defaultOpacity
			}

			if (parent.pressed) {
				return 1
			}

			if (parent.hovered) {
				return root.strongBackgroundOpacityChange ? 0.5 : 0.9
			}

			if (parent.checked) {
				return 0.35
			}

			return defaultOpacity
		}
		radius: height / 2

		Behavior on opacity {
			NumberAnimation {}
		}
	}
	contentItem: Kirigami.Icon {
		id: icon
		source: root.iconSource
		isMask: true
		implicitWidth: root.iconSize
		implicitHeight: root.iconSize
		Kirigami.Theme.colorSet: Kirigami.Theme.Window
		Kirigami.Theme.inherit: false
	}
	horizontalPadding: iconSize * 0.5
	verticalPadding: horizontalPadding
	topInset: 0
	bottomInset: 0
	leftInset: 0
	rightInset: 0
}
