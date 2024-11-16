// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15
import QtQuick.Controls 2.15 as Controls
import org.kde.kirigami 2.19 as Kirigami

/**
 * This is a slider to zoom in/out to be used as an overlay on top of a visual item.
 */
Slider {
	id: root
	value: 1
	from: 1
	to: 3
	width: parent.width - Kirigami.Units.largeSpacing * 4
	horizontalPadding: Kirigami.Units.largeSpacing
	anchors.bottom: parent.bottom
	anchors.bottomMargin: Kirigami.Units.largeSpacing * 3
	anchors.horizontalCenter: parent.horizontalCenter
	background: Rectangle {
		color: primaryBackgroundColor
		opacity: 0.9
		radius: relativeRoundedCornersRadius(parent.width, parent.height) * 3
		height: implicitHeight
		implicitHeight: Kirigami.Units.largeSpacing * 4
		anchors.verticalCenter: parent.verticalCenter

		SliderBackground {
			visualPosition: root.visualPosition
			width: parent.width - root.horizontalPadding * 2
			anchors.left: parent.left
			anchors.leftMargin: root.horizontalPadding
		}
	}
}
