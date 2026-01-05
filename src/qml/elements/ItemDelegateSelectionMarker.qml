// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

import im.kaidan.kaidan

/**
 * Used to select a Controls.ItemDelegate inside of it.
 */
Rectangle {
	id: root

	property Controls.ItemDelegate itemDelegate

	color: {
		let colorOpacity = 0

		if (itemDelegate.pressed) {
			colorOpacity = 0.35
		} else if (itemDelegate.checked) {
			if (itemDelegate.hovered) {
				colorOpacity = 0.3
			} else {
				colorOpacity = 0.25
			}
		} else if (itemDelegate.hovered) {
			colorOpacity = 0.1
		}

		const textColor = Kirigami.Theme.textColor
		return Qt.rgba(textColor.r, textColor.g, textColor.b, colorOpacity)
	}
	implicitWidth: Kirigami.Units.iconSizes.smallMedium
	implicitHeight: Kirigami.Units.iconSizes.smallMedium
	radius: implicitWidth

	Behavior on color {
		ColorAnimation { duration: Kirigami.Units.shortDuration }
	}

	Kirigami.Icon {
		source: "emblem-ok-symbolic"
		color: Kirigami.Theme.backgroundColor
		implicitWidth: Kirigami.Units.iconSizes.small
		implicitHeight: Kirigami.Units.iconSizes.small
		visible: root.itemDelegate.hovered || root.itemDelegate.checked
		anchors.centerIn: parent
	}
}
