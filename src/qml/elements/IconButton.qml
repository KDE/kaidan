// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * Round icon-only button with a background changing its color on interaction and a tooltip.
 */
Controls.AbstractButton {
	id: root

	property color backgroundColor: Kirigami.Theme.textColor
	property bool flat: true
	property bool _longPressed: false

	hoverEnabled: true
	visible: opacity
	icon.width: Kirigami.Units.iconSizes.small
	background: Kirigami.ShadowedRectangle {
		color: interactiveBackgroundColor(parent, root.flat)
		radius: height / 2

		Behavior on color {
			ColorAnimation {
				duration: Kirigami.Units.shortDuration
			}
		}
	}
	contentItem: Kirigami.Icon {
		source: root.icon.source
		color: root.icon.color
		isMask: !Qt.colorEqual(color, "transparent")
		implicitWidth: root.icon.width
		implicitHeight: root.icon.height
		Kirigami.Theme.colorSet: Kirigami.Theme.Window
		Kirigami.Theme.inherit: false
	}
	horizontalPadding: Math.round(icon.width * 0.5)
	verticalPadding: Math.round(icon.height * 0.5)
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

	Behavior on opacity {
		NumberAnimation {}
	}
}
