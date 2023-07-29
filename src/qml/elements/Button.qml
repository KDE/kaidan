// SPDX-FileCopyrightText: 2020 Melvin Keskin <melvo@olomono.de>
// SPDX-FileCopyrightText: 2021 Linus Jahn <lnj@kaidan.im>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * This is a button fitting mobile and desktop user interfaces.
 */
Controls.Button {
	property bool remainTooltip: false

	flat: Style.isMaterial
	hoverEnabled: true
	Controls.ToolTip.delay: Kirigami.Settings.isMobile ? 0 : Kirigami.Units.veryLongDuration * 2
	Controls.ToolTip.timeout: Kirigami.Units.veryLongDuration * 10
	onHoveredChanged: {
		if (Controls.ToolTip.text && !Kirigami.Settings.isMobile) {
			if (hovered) {
				Controls.ToolTip.show(Controls.ToolTip.text, Controls.ToolTip.timeout)
			} else {
				Controls.ToolTip.hide()
			}
		}
	}
	onPressed: {
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
				Controls.ToolTip.show(Controls.ToolTip.text, Controls.ToolTip.timeout)
			}
		}
	}
}
