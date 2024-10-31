// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick 2.15

/**
 * This is placed inside of an item to change the opacity of opacityItem when the item is hovered.
 */
MouseArea {
	property Item opacityItem

	hoverEnabled: true
	anchors.fill: parent
	onContainsMouseChanged: opacityItem.opacity = containsMouse ? 0.5 : 1
}
