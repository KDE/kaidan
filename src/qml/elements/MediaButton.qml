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
	property bool _longPressed: false

	hoverEnabled: true
	icon.width: Kirigami.Units.iconSizes.small
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
		source: root.icon.source
		color: root.icon.color
		isMask: true
		implicitWidth: root.icon.width
		implicitHeight: root.icon.height
		Kirigami.Theme.colorSet: Kirigami.Theme.Window
		Kirigami.Theme.inherit: false
	}
	horizontalPadding: icon.width * 0.5
	verticalPadding: icon.height * 0.5
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

			if (checkable) {
				toggle()
			} else {
				clicked()
			}
		}
	}
	Component.onCompleted: icon.height = icon.width
}
