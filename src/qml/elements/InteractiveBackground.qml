// SPDX-FileCopyrightText: 2026 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import org.kde.kirigami as Kirigami

/**
 * Background for a visual feedback on an item that a user interacts with.
 */
Rectangle {
	property Item interactionItem: parent
	property bool flat: true
	property bool inverted: false

	radius: roundedCornersRadius
	color: interactiveBackgroundColor(interactionItem, flat, inverted)

	Behavior on color {
		ColorAnimation {
			duration: Kirigami.Units.shortDuration
		}
	}
}
