// SPDX-FileCopyrightText: 2025 Melvin Keskin <melvo@olomono.de>
//
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard

/**
 * This is a background for a checkable form delegate that is used to expand a card by displaying
 * formerly hidden delegates below the checkable form delegate.
 */
Kirigami.ShadowedRectangle {
	/**
	 * Checkable form delegate to expand the card.
	 */
	required property FormCard.AbstractFormDelegate button

	/**
	 * Card that is expanded.
	 */
	required property FormCard.FormCard card

	corners {
		readonly property bool roundedTopCorners: card.cardWidthRestricted
		readonly property bool roundedBottomCorners: card.cardWidthRestricted && !button.parent.children[1].height

		topLeftRadius: roundedTopCorners ? Kirigami.Units.smallSpacing : 0
		topRightRadius: roundedTopCorners ? Kirigami.Units.smallSpacing : 0
		bottomLeftRadius: roundedBottomCorners ? Kirigami.Units.smallSpacing : 0
		bottomRightRadius: roundedBottomCorners ? Kirigami.Units.smallSpacing : 0
	}
	color: {
		let colorOpacity = 0

		if (!button.enabled) {
			colorOpacity = 0
		} else if (button.pressed) {
			colorOpacity = 0.2
		} else if (button.visualFocus) {
			colorOpacity = 0.1
		} else if (!Kirigami.Settings.tabletMode && button.hovered) {
			colorOpacity = 0.07
		} else if (button.checked) {
			colorOpacity = 0.05
		}

		const textColor = Kirigami.Theme.textColor
		return Qt.rgba(textColor.r, textColor.g, textColor.b, colorOpacity)
	}

	Behavior on color {
		ColorAnimation {
			duration: Kirigami.Units.shortDuration
		}
	}
}
