// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as Controls
import org.kde.kirigami as Kirigami

/**
 * This is a custom background used for a slider.
 */
Rectangle {
	id: root

	required property real visualPosition

	color: Kirigami.Theme.activeBackgroundColor
	radius: height / 2
	height: implicitHeight
	implicitHeight: Kirigami.Units.smallSpacing
	anchors.verticalCenter: parent.verticalCenter

	Rectangle {
		color: Kirigami.Theme.activeTextColor
		opacity: 0.8
		radius: parent.radius
		width: root.visualPosition * parent.width
		height: parent.height
	}
}
