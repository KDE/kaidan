// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a mouse area with tooltip functionality and color changes on several events if possible.
 *
 * The parent of this item needs to have a "color" property and a "clicked" signal.
 */
MouseArea {
	property bool remainTooltip: false

	anchors.fill: parent
	hoverEnabled: true
	Controls.ToolTip.text: parent.Controls.ToolTip.text
	Controls.ToolTip.delay: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.veryLongDuration * 2
	Controls.ToolTip.timeout: Kirigami.Units.veryLongDuration * 10
	onEntered: {
		parent.color = Kirigami.Theme.hoverColor

		if (Controls.ToolTip.text && !Kirigami.Settings.isMobile) {
			Controls.ToolTip.visible = true
		}
	}
	onPressed: {
		parent.color = Kirigami.Theme.focusColor

		if (Controls.ToolTip.text && !Kirigami.Settings.isMobile) {
			Controls.ToolTip.visible = false
		}
	}
	onPressAndHold: {
		if (Controls.ToolTip.text && Kirigami.Settings.isMobile) {
			remainTooltip = true
			Controls.ToolTip.visible = true
		}
	}
	onReleased: {
		if (Controls.ToolTip.text && Kirigami.Settings.isMobile) {
			if (remainTooltip) {
				remainTooltip = false
				// TODO: Remain tooltip on mobile devices
				Controls.ToolTip.visible = true
			}
		}
	}
	onExited: {
		parent.color = Kirigami.Theme.textColor
	}
	onClicked: {
		parent.clicked()

		if (containsMouse) {
			entered()
		} else {
			exited()
		}
	}
}
