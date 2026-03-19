// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

/**
 * This is placed inside of an item to change the opacity of opacityItem when the item is hovered.
 */
MouseArea {
	id: root

	property Item opacityItem
	property real baseOpacity: 1
	property real hoverOpacity: 0.5
	property alias opacityBinding: opacityBinding

	hoverEnabled: true
	anchors.fill: parent

	Binding {
		id: opacityBinding

		target: opacityItem
		property: 'opacity'
		value: root.containsMouse ? root.hoverOpacity : root.baseOpacity
	}
}
