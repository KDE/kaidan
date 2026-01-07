// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

/**
 * This is a mouse area for media to change the opacity of opacityItem when it is hovered or selected.
 */
OpacityChangingMouseArea {
	property bool selected: false

	onContainsMouseChanged: opacityItem.opacity = selected || containsMouse ? 0.5 : 1
	onSelectedChanged: opacityItem.opacity = selected || containsMouse ? 0.5 : 1
}
