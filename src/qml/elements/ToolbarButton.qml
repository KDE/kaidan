// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * This is a clickable icon to be used inside of the toolbar.
 */
ClickableIcon  {
	property bool checkable: false
	property bool checked: false

	active: mouseArea.containsMouse || checked
	isMask: true
	onClicked: {
		if (checkable) {
			checked = !checked
		}
	}
}
