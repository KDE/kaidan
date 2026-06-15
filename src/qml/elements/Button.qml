// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Base for round buttons with a background changing its color on interaction and a tooltip.
 */
Controls.AbstractButton {
	id: root

	property bool flat: true
	property bool inverted: false
	property bool longPressBehaviorEnabled: true
	property bool _longPressed: false

	hoverEnabled: true
	visible: opacity
	background: InteractiveBackground {
		flat: root.flat
		inverted: root.inverted
		radius: height / 2
	}
	Controls.ToolTip.text: text
	Controls.ToolTip.visible: enabled && !pressed && hovered
	Controls.ToolTip.delay: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.veryLongDuration * 2
	Controls.ToolTip.timeout: Kirigami.Units.veryLongDuration * 10
	Kirigami.Theme.colorSet: Kirigami.Theme.Window
	Kirigami.Theme.inherit: false
	onClicked: Controls.ToolTip.hide()
	onPressAndHold: {
		if (longPressBehaviorEnabled) {
			_longPressed = true

			if (Kirigami.Settings.isMobile) {
				Controls.ToolTip.visible = true
			}
		}
	}
	onReleased: {
		// Simulate clicking if the mouse cursor is still on the button while releasing after a long press.
		// That is needed because if "onPressAndHold" is used, the desired functionality does not work anymore.
		if (longPressBehaviorEnabled && !Kirigami.Settings.isMobile && _longPressed) {
			_longPressed = false

			if (checkable) {
				toggle()
			} else {
				clicked()
			}
		}
	}

	Behavior on opacity {
		NumberAnimation {}
	}
}
