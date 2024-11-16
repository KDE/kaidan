// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a slider with a custom background and handle.
 */
Controls.Slider {
	id: root
	from: 0
	implicitHeight: Kirigami.Units.iconSizes.small
	background: SliderBackground {
		visualPosition: parent.visualPosition
	}
	handle: Rectangle {
		x: parent.leftPadding + parent.visualPosition * (parent.availableWidth - width)
		color: {
			const baseColor = Kirigami.Theme.activeTextColor

			if (parent.pressed) {
				return Qt.darker(baseColor, 1.4)
			}

			if (parent.hovered) {
				return Qt.darker(baseColor, 1.2)
			}

			return baseColor
		}
		radius: height / 2
		implicitWidth: Kirigami.Units.iconSizes.small
		implicitHeight: implicitWidth
		anchors.verticalCenter: parent.verticalCenter

		Behavior on color {
			ColorAnimation {}
		}
	}
}
