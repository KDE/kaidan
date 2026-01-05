// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * This is a button to be used for calls inside of the toolbar.
 */
ToolbarButton  {
	id: root

	Component.onCompleted: adaptColor()
	mouseArea.onExited: adaptColor()

	Connections {
		target: MainController

		function onActiveCallChanged() {
			adaptColor()
		}
	}

	function adaptColor() {
		if (MainController.activeCall) {
			root.color = Kirigami.Theme.positiveTextColor
		} else {
			root.color = Kirigami.Theme.disabledTextColor
		}
	}
}
