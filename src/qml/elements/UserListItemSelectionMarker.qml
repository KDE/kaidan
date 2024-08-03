// SPDX-FileCopyrightText: 2023 Filipe Azevedo <pasnox@gmail.com>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.14
import QtQuick.Controls 2.14 as Controls
import org.kde.kirigami 2.19 as Kirigami

import im.kaidan.kaidan 1.0

/**
 * Used to select a UserlistItem inside of it.
 */
Rectangle {
	property Item userListItem

	color: {
		let colorOpacity = 0;

		if (userListItem.pressed) {
			colorOpacity = 0.35
		} else if (userListItem.selected) {
			if (userListItem.hovered) {
				colorOpacity = 0.3
			} else {
				colorOpacity = 0.25
			}
		} else if (userListItem.hovered) {
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
		anchors.centerIn: parent
	}
}
