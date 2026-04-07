// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

Controls.AbstractButton {
	id: root

	property color backgroundColor: secondaryBackgroundColor
	property bool strongBackgroundOpacityChange: false
	property var iconSource
	property double iconSize: Kirigami.Units.iconSizes.small
	property alias iconColor: icon.color
	property bool _longPressed: false

	hoverEnabled: true
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
	Controls.ToolTip.visible: enabled && !pressed && hovered
	Controls.ToolTip.delay: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.veryLongDuration * 2
	Controls.ToolTip.timeout: Kirigami.Units.veryLongDuration * 10
	onClicked: Controls.ToolTip.hide()
	onPressAndHold: {
		_longPressed = true

		if (Kirigami.Settings.isMobile) {
			Controls.ToolTip.visible = true
		}
	}
	onReleased: {
		// Simulate clicking if the mouse cursor is still on the button while releasing after a long press.
		// That is needed because if "onPressAndHold" is used, the desired functionality does not work anymore.
		if (!Kirigami.Settings.isMobile && _longPressed) {
			_longPressed = false
			clicked()
		}
	}
}
