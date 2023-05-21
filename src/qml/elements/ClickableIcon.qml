// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a clickable icon changing its color on several events if possible.
 */
Kirigami.Icon  {
	width: Kirigami.Units.largeSpacing * 3.8
	height: width

	// Icons which cannot be colored (e.g. emojis) are highlighted in another way.
	active: mouseArea.containsMouse

	property alias mouseArea: mouseArea
	signal clicked()

	MouseArea {
		id: mouseArea

		property bool remainTooltip: false

		anchors.fill: parent
		hoverEnabled: true
		cursorShape: Qt.PointingHandCursor
		Controls.ToolTip.text: parent.Controls.ToolTip.text
		Controls.ToolTip.delay: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.veryLongDuration * 2
		Controls.ToolTip.timeout: Kirigami.Units.veryLongDuration * 10
		onEntered: {
			parent.color = Kirigami.Theme.hoverColor

			if (Controls.ToolTip.text && !Kirigami.Settings.isMobile) {
				Controls.ToolTip.show(Controls.ToolTip.text, Controls.ToolTip.timeout)
			}
		}
		onPressed: {
			parent.color = Kirigami.Theme.focusColor

			if (Controls.ToolTip.text && !Kirigami.Settings.isMobile) {
				Controls.ToolTip.hide()
			}
		}
		onPressAndHold: {
			if (Controls.ToolTip.text && Kirigami.Settings.isMobile) {
				remainTooltip = true
				Controls.ToolTip.show(Controls.ToolTip.text, Controls.ToolTip.timeout)
			}
		}
		onReleased: {
			if (Controls.ToolTip.text && Kirigami.Settings.isMobile) {
				if (remainTooltip) {
					remainTooltip = false
					// TODO: Remain tooltip on mobile devices
					Controls.ToolTip.show(Controls.ToolTip.text, Controls.ToolTip.timeout)
				}
			}
		}
		onExited: {
			parent.color = Kirigami.Theme.textColor
		}
		onClicked: {
			parent.clicked()

			if (containsMouse)
				entered()
			else
				exited()
		}
	}
}
