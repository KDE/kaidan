// SPDX-FileCopyrightText: 2024 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick

/**
 * This is a mouse area for media to change the opacity of opacityItem when it is hovered or selected.
 */
OpacityChangingMouseArea {
	property bool selected: false

	opacityBinding.value: selected || containsMouse ? hoverOpacity : baseOpacity
}
